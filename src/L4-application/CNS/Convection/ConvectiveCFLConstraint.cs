﻿/* =======================================================================
Copyright 2017 Technische Universitaet Darmstadt, Fachgebiet fuer Stroemungsdynamik (chair of fluid dynamics)

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
*/

using BoSSS.Foundation.Grid.Classic;
using BoSSS.Foundation.XDG;
using BoSSS.Platform.LinAlg;
using CNS.EquationSystem;
using CNS.IBM;
using CNS.MaterialProperty;
using ilPSP;
using System;
using System.Diagnostics;
using System.Linq;

namespace CNS.Convection {

    /// <summary>
    /// Implementation of the CFL constraint induced by the convective terms of
    /// the compressible Navier-Stokes equations
    /// </summary>
    public class ConvectiveCFLConstraint : CFLConstraint {

        /// <summary>
        /// <see cref="ConvectiveCFLConstraint.ConvectiveCFLConstraint"/>
        /// </summary>
        private CNSControl control;

        /// <summary>
        /// <see cref="ConvectiveCFLConstraint.ConvectiveCFLConstraint"/>
        /// </summary>
        private ISpeciesMap speciesMap;

        /// <summary>
        /// Constructs a new constraint
        /// </summary>
        /// <param name="config"></param>
        /// <param name="gridData"></param>
        /// <param name="workingSet"></param>
        /// <param name="speciesMap"></param>
        public ConvectiveCFLConstraint(
            CNSControl config, GridData gridData, CNSFieldSet workingSet, ISpeciesMap speciesMap)
            : base(gridData, workingSet) {

            this.control = config;
            this.speciesMap = speciesMap;

            if (gridData.Grid.RefElements.Length > 1) {
                throw new NotImplementedException();
            }
        }

        /// <summary>
        /// Computes the maximum admissible step-size according to
        /// \f$ 
        /// \text{cfl} = \frac{1}{2N+1} \min \frac{h}{|\vec{u}| + a} 
        /// \f$ 
        /// where \f$ h\f$  is the minimum cell diameter,
        /// \f$ \vec{u}\f$  is the nodal velocity vector
        /// and \f$ a\f$  is the nodal speed of sound.
        /// Additionally, \f$ N\f$  is the degree of the
        /// DG with the highest degree of approximation.
        /// </summary>
        /// <param name="i0"></param>
        /// <param name="Length"></param>
        /// <returns></returns>
        protected override double GetCFLStepSize(int i0, int Length) {
            int iKref = gridData.Cells.GetRefElementIndex(i0);
            int noOfNodesPerCell = base.EvaluationPoints[iKref].NoOfNodes;
            int D = gridData.Grid.SpatialDimension;
            Material material = speciesMap.GetMaterial(double.NaN);
            MultidimensionalArray hmin = gridData.Cells.h_min;
            double gamma = material.EquationOfState.HeatCapacityRatio;
            double Ma = material.Control.MachNumber;

            // Following code is performance-critical, so expect spagetti code ahead
            double cfl = double.MaxValue;
            switch (speciesMap) {
                // 1) IBM optimized (ignores nodes in the void part of a cell, only applies to ideal gases)
                case ImmersedSpeciesMap ibmMap when material.EquationOfState is IdealGas: {
                        MultidimensionalArray levelSetValues =
                            ibmMap.Tracker.GetLevSetValues(0, base.EvaluationPoints[iKref], i0, Length);

                        SpeciesId species = ibmMap.Tracker.GetSpeciesId(ibmMap.Control.FluidSpeciesName);
                        //var hMin = speciesMap.QuadSchemeHelper.CellAgglomeration.CellLengthScales[species];
                        var volFrac = ibmMap.QuadSchemeHelper.CellAgglomeration.CellVolumeFrac[species];

                        for (int i = 0; i < Length; i++) {
                            int cell = i0 + i;

                            // Option 1: Use volume fraction times traditional metric, such
                            // that time-steps are identical to non-IBM cases when no
                            // interface is present
                            double hminlocal = hmin[cell] * volFrac[cell];

                            // Option 2: Use length scale "volume over surface", which seems
                            // to be more robust for awkward cuts. However, yields
                            // significantly smaller time-steps in uncut cells
                            //double hminlocal = hMin[cell];

                            for (int node = 0; node < noOfNodesPerCell; node++) {
                                if (levelSetValues[i, node].Sign() != (double)ibmMap.Control.FluidSpeciesSign) {
                                    continue;
                                }

                                double momentumSquared = 0.0;
                                for (int d = 0; d < D; d++) {
                                    momentumSquared += momentumValues[d][i, node] * momentumValues[d][i, node];
                                }

                                double density = densityValues[i, node];
                                double energy = energyValues[i, node];

                                // Following quantities need scaling according to
                                // non -dimensionalization, see StateVector/IdealGas
                                double kineticEnergy = 0.5 * momentumSquared / density * gamma * Ma * Ma;
                                double sos = Math.Sqrt((gamma - 1.0) * (energy - kineticEnergy)) / Ma;

                                double flowSpeed = Math.Sqrt(momentumSquared) / density;
                                double cflhere = hmin[cell] / (flowSpeed + sos);

#if DEBUG
                                if (double.IsNaN(cflhere)) {
                                    throw new Exception("Could not determine CFL number");
                                }
#endif

                                cfl = Math.Min(cfl, cflhere);
                            }
                        }
                    }
                    break;

                // 2) IBM generic (ignores nodes in the void part of a cell)
                case ImmersedSpeciesMap ibmMap: {
                        MultidimensionalArray levelSetValues =
                            ibmMap.Tracker.GetLevSetValues(0, base.EvaluationPoints[iKref], i0, Length);

                        SpeciesId species = ibmMap.Tracker.GetSpeciesId(ibmMap.Control.FluidSpeciesName);
                        //var hMin = speciesMap.QuadSchemeHelper.CellAgglomeration.CellLengthScales[species];
                        var volFrac = ibmMap.QuadSchemeHelper.CellAgglomeration.CellVolumeFrac[species];

                        for (int i = 0; i < Length; i++) {
                            int cell = i0 + i;

                            // Option 1: Use volume fraction times traditional metric, such
                            // that time-steps are identical to non-IBM cases when no
                            // interface is present
                            double hminlocal = hmin[cell] * volFrac[cell];

                            // Option 2: Use length scale "volume over surface", which seems
                            // to be more robust for awkward cuts. However, yields
                            // significantly smaller time-steps in uncut cells
                            //double hminlocal = hMin[cell];

                            for (int node = 0; node < noOfNodesPerCell; node++) {
                                if (levelSetValues[i, node].Sign() != (double)ibmMap.Control.FluidSpeciesSign) {
                                    continue;
                                }

                                Vector3D momentum = new Vector3D();
                                for (int d = 0; d < CNSEnvironment.NumberOfDimensions; d++) {
                                    momentum[d] = momentumValues[d][i, node];
                                }

                                StateVector state = new StateVector(
                                    material, densityValues[i, node], momentum, energyValues[i, node]);
                                double cflhere = hminlocal / (state.Velocity.Abs() + state.SpeedOfSound);

#if DEBUG
                                if (double.IsNaN(cflhere)) {
                                    throw new Exception("Could not determine CFL number");
                                }
#endif

                                cfl = Math.Min(cfl, cflhere);
                            }
                        }
                    }
                    break;

                // 3) Non-IBM optimized (only applies to ideal gases)
                case var speciesMap when material.EquationOfState is IdealGas:
                    for (int i = 0; i < Length; i++) {
                        int cell = i0 + i;

                        for (int node = 0; node < noOfNodesPerCell; node++) {
                            double momentumSquared = 0.0;
                            for (int d = 0; d < D; d++) {
                                momentumSquared += momentumValues[d][i, node] * momentumValues[d][i, node];
                            }

                            double density = densityValues[i, node];
                            double energy = energyValues[i, node];

                            // Following quantities need scaling according to
                            // non -dimensionalization, see StateVector/IdealGas
                            double kineticEnergy = 0.5 * momentumSquared / density * gamma * Ma * Ma;
                            double sos = Math.Sqrt((gamma - 1.0) * (energy - kineticEnergy) / density) / Ma;

                            double flowSpeed = Math.Sqrt(momentumSquared) / density;
                            double cflhere = hmin[cell] / (flowSpeed + sos);

#if DEBUG
                            if (double.IsNaN(cflhere)) {
                                throw new Exception("Could not determine CFL number");
                            }

                            Vector3D momentum = new Vector3D();
                            for (int d = 0; d < CNSEnvironment.NumberOfDimensions; d++) {
                                momentum[d] = momentumValues[d][i, node];
                            }
                            StateVector state = new StateVector(
                                material, densityValues[i, node], momentum, energyValues[i, node]);
                            double cflgeneric = hmin[cell] /
                                (state.Velocity.Abs() + material.EquationOfState.GetSpeedOfSound(state));
                            if (Math.Abs(cflgeneric - cflhere) > 1e-15) {
                                throw new Exception("Inconsistency in optimized evaluation of cfl number");
                            }
#endif

                            cfl = Math.Min(cfl, cflhere);
                        }
                    }
                    break;

                // 4) Non-IBM generic (works for all equations of state)
                default: {
                        for (int i = 0; i < Length; i++) {
                            int cell = i0 + i;

                            for (int node = 0; node < noOfNodesPerCell; node++) {
                                Vector3D momentum = new Vector3D();
                                for (int d = 0; d < CNSEnvironment.NumberOfDimensions; d++) {
                                    momentum[d] = momentumValues[d][i, node];
                                }

                                StateVector state = new StateVector(
                                    material, densityValues[i, node], momentum, energyValues[i, node]);
                                double cflhere = hmin[cell] /
                                    (state.Velocity.Abs() + material.EquationOfState.GetSpeedOfSound(state));

#if DEBUG
                                if (double.IsNaN(cflhere)) {
                                    throw new Exception("Could not determine CFL number");
                                }
#endif

                                cfl = Math.Min(cfl, cflhere);
                            }
                        }
                    }
                    break;
            }

            int twoNPlusOne = 2 * workingSet.ConservativeVariables.Max(f => f.Basis.Degree) + 1;
            return cfl / twoNPlusOne;
        }
    }
}