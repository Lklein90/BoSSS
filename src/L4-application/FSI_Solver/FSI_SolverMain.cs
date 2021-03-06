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

using System;
using System.Collections.Generic;
using System.Linq;
using BoSSS.Foundation.XDG;
using BoSSS.Solution.NSECommon;
using BoSSS.Solution;
using BoSSS.Foundation;
using BoSSS.Foundation.Grid;
using BoSSS.Solution.Tecplot;
using ilPSP.Utils;
using ilPSP.Tracing;
using BoSSS.Platform;
using ilPSP.LinSolvers;
using BoSSS.Solution.Utils;
using BoSSS.Solution.LevelSetTools.Smoothing;
using BoSSS.Foundation.SpecFEM;
using MPI.Wrappers;
using BoSSS.Foundation.IO;
using System.Diagnostics;
using System.IO;
using BoSSS.Foundation.Quadrature;
using BoSSS.Solution.Multigrid;
using ilPSP;
using BoSSS.Solution.XdgTimestepping;
using BoSSS.Foundation.Grid.RefElements;

namespace BoSSS.Application.FSI_Solver {
    public class FSI_SolverMain : IBM_Solver.IBM_SolverMain {
        static void Main(string[] args) {

            _Main(args, false, delegate () {
                var p = new FSI_SolverMain();
                return p;
            });
        }

        protected override void CreateEquationsAndSolvers(GridUpdateDataVaultBase L) {


            if (IBM_Op != null)
                return;

            bool UseMovingMesh = false;

            if (((FSI_Control)this.Control).Timestepper_Mode == FSI_Control.TimesteppingMode.MovingMesh)
                UseMovingMesh = true;

            string[] CodNameSelected = new string[0];
            string[] DomNameSelected = new string[0];

            int D = this.GridData.SpatialDimension;


            BcMap = new IncompressibleBoundaryCondMap(this.GridData, this.Control.BoundaryValues, PhysicsMode.Incompressible);
            // BcMap = new IncompressibleMultiphaseBoundaryCondMap(this.GridData, this.Control.BoundaryValues, LsTrk.SpeciesNames.ToArray());

            int degU = this.Velocity[0].Basis.Degree;
            var IBM_Op_config = new NSEOperatorConfiguration {
                convection = this.Control.PhysicalParameters.IncludeConvection,
                continuity = true,
                Viscous = true,
                PressureGradient = true,
                Transport = true,
                CodBlocks = new bool[] { true, true },
                DomBlocks = new bool[] { true, true },
            };

            // full operator:
            var CodName = ((new string[] { "momX", "momY", "momZ" }).GetSubVector(0, D)).Cat("div");
            var Params = ArrayTools.Cat(
                 VariableNames.Velocity0Vector(D),
                 VariableNames.Velocity0MeanVector(D));
            var DomName = ArrayTools.Cat(VariableNames.VelocityVector(D), VariableNames.Pressure);

            // selected part:
            if (IBM_Op_config.CodBlocks[0])
                CodNameSelected = ArrayTools.Cat(CodNameSelected, CodName.GetSubVector(0, D));
            if (IBM_Op_config.CodBlocks[1])
                CodNameSelected = ArrayTools.Cat(CodNameSelected, CodName.GetSubVector(D, 1));

            if (IBM_Op_config.DomBlocks[0])
                DomNameSelected = ArrayTools.Cat(DomNameSelected, DomName.GetSubVector(0, D));
            if (IBM_Op_config.DomBlocks[1])
                DomNameSelected = ArrayTools.Cat(DomNameSelected, DomName.GetSubVector(D, 1));

            IBM_Op = new XSpatialOperator(DomNameSelected, Params, CodNameSelected,
                (A, B, C) => this.HMForder
                );


            // Momentum equation
            // =================

            // convective part:
            if (IBM_Op_config.convection) {
                for (int d = 0; d < D; d++) {

                    var comps = IBM_Op.EquationComponents[CodName[d]];

                    //var ConvBulk = new Solution.XNSECommon.Operator.Convection.ConvectionInBulk_LLF(D, BcMap, d, this.Control.PhysicalParameters.rho_A, 0, this.Control.AdvancedDiscretizationOptions.LFFA, this.Control.AdvancedDiscretizationOptions.LFFB, LsTrk);
                    var ConvBulk = new Solution.NSECommon.LinearizedConvection(D, BcMap, d);
                    //IBM_Op.OnIntegratingBulk += ConvBulk.SetParameter;
                    comps.Add(ConvBulk); // bulk component

                    //var ConvIB = new BoSSS.Solution.XNSECommon.Operator.Convection.ConvectionAtIB(d, D, LsTrk, IBM_Op_config.dntParams.LFFA, BcMap, uIBM, wIBM);

                    if (((FSI_Control)this.Control).LevelSetMovement.ToLowerInvariant() == "fixed") {

                        var ConvIB = new BoSSS.Solution.NSECommon.Operator.Convection.ConvectionAtIB(d, D, LsTrk,
                            this.Control.AdvancedDiscretizationOptions.LFFA, BcMap,
                            delegate (double[] X, double time) {
                                throw new NotImplementedException("Currently not implemented for fixed motion");
                                //return new double[] { 0.0, 0.0 };
                            },
                            this.Control.PhysicalParameters.rho_A,
                            UseMovingMesh);
                        comps.Add(ConvIB); // immersed boundary component
                    } else {
                        var ConvIB = new BoSSS.Solution.NSECommon.Operator.Convection.ConvectionAtIB(d, D, LsTrk,
                            this.Control.AdvancedDiscretizationOptions.LFFA, BcMap,
                                delegate (double[] X, double time) {

                                    double[] result = new double[X.Length + 2];

                                    foreach (Particle p in m_Particles) {
                                        bool containsParticle;
                                        if (m_Particles.Count == 1) {
                                            containsParticle = true;
                                        } else { containsParticle = p.Contains(X); }


                                        if (containsParticle) {
                                            result[0] = p.vel_P[0][0];
                                            result[1] = p.vel_P[0][1];
                                            result[2] = p.rot_P[0];
                                            result[3] = p.currentPos_P[0].L2Distance(X);
                                            return result;
                                        }
                                    }
                                    return result;
                                },
                            this.Control.PhysicalParameters.rho_A,
                            UseMovingMesh);
                        comps.Add(ConvIB); // immersed boundary component
                    }

                }
                this.U0MeanRequired = true;
            }

            // pressure part:
            if (IBM_Op_config.PressureGradient) {
                for (int d = 0; d < D; d++) {
                    var comps = IBM_Op.EquationComponents[CodName[d]];
                    //var pres = new Solution.XNSECommon.Operator.Pressure.PressureInBulk(d, BcMap, 1, 0);
                    var pres = new PressureGradientLin_d(d, BcMap);
                    //IBM_Op.OnIntegratingBulk += pres.SetParameter;
                    comps.Add(pres); // bulk component

                    var presLs = new BoSSS.Solution.NSECommon.Operator.Pressure.PressureFormAtIB(d, D, LsTrk);
                    comps.Add(presLs); // immersed boundary component

                    // if periodic boundary conditions are applied a fixed pressure gradient drives the flow
                    if (this.Control.FixedStreamwisePeriodicBC) {
                        var presSource = new SrcPressureGradientLin_d(this.Control.SrcPressureGrad[d]);
                        comps.Add(presSource);
                    }
                }
            }

            // viscous part:
            if (IBM_Op_config.Viscous) {
                for (int d = 0; d < D; d++) {
                    var comps = IBM_Op.EquationComponents[CodName[d]];
                    //double _D = D;
                    //double penalty_mul = this.Control.AdvancedDiscretizationOptions.PenaltySafety;
                    //double _p = degU;
                    //double penalty_base = (_p + 1) * (_p + _D) / D;
                    //double penalty = penalty_base * penalty_mul;
                    double penalty = this.Control.AdvancedDiscretizationOptions.PenaltySafety;


                    var Visc = new swipViscosity_Term1(penalty, d, D, BcMap, ViscosityOption.ConstantViscosity, this.Control.PhysicalParameters.mu_A / this.Control.PhysicalParameters.rho_A, double.NaN, null);

                    comps.Add(Visc);


                    //var Visc = new Solution.XNSECommon.Operator.Viscosity.ViscosityInBulk_GradUTerm(penalty, 1.0, BcMap, d, D, this.Control.PhysicalParameters.mu_A, 0, ViscosityImplementation.H);
                    //IBM_Op.OnIntegratingBulk += Visc.SetParameter;
                    //comps.Add(Visc); // bulk component GradUTerm

                    //delegate (double p, int i, int j, double[] cell) { return ComputePenalty(p, i, j, cell); });
                    //delegate (double p, int i, int j, double[] cell) { return ComputePenalty(p, i, j, cell); });
                    //FSI_Op.OnIntegratingBulk += Visc.SetParameter;                

                    if (((FSI_Control)this.Control).LevelSetMovement.ToLowerInvariant() == "fixed") {

                        var ViscLs = new BoSSS.Solution.NSECommon.Operator.Viscosity.ViscosityAtIB(d, D, LsTrk,
                            penalty, this.ComputePenaltyIB,
                            this.Control.PhysicalParameters.mu_A / this.Control.PhysicalParameters.rho_A,
                            delegate (double[] X, double time) {
                                throw new NotImplementedException("Currently not implemented for fixed motion");
                                //return new double[] { 0.0, 0.0 };
                            });
                        comps.Add(ViscLs); // immersed boundary component
                    } else {

                        var ViscLs = new BoSSS.Solution.NSECommon.Operator.Viscosity.ViscosityAtIB(d, D, LsTrk,
                            penalty, this.ComputePenaltyIB,
                            this.Control.PhysicalParameters.mu_A / this.Control.PhysicalParameters.rho_A,
                            delegate (double[] X, double time) {

                                double[] result = new double[X.Length + 2];

                                foreach (Particle p in m_Particles) {
                                    bool containsParticle;
                                    if (m_Particles.Count == 1) {
                                        containsParticle = true;
                                    } else { containsParticle = p.Contains(X); }


                                    if (containsParticle) {
                                        result[0] = p.vel_P[0][0];
                                        result[1] = p.vel_P[0][1];
                                        result[2] = p.rot_P[0];
                                        result[3] = p.currentPos_P[0].L2Distance(X);
                                        return result;
                                    }
                                }
                                return result;
                            });
                        comps.Add(ViscLs); // immersed boundary component
                    }
                }
            }

            // Continuum equation
            // ==================
            if (IBM_Op_config.continuity) {
                for (int d = 0; d < D; d++) {
                    //var src = new Solution.XNSECommon.Operator.Continuity.DivergenceInBulk_Volume(d, D, 1, 0, 1, false);
                    //IBM_Op.OnIntegratingBulk += src.SetParameter;
                    //var flx = new Solution.XNSECommon.Operator.Continuity.DivergenceInBulk_Edge(d, BcMap, 1, 0, 1, false);
                    //IBM_Op.OnIntegratingBulk += flx.SetParameter;
                    var src = new Divergence_DerivativeSource(d, D);
                    //IBM_Op.OnIntegratingBulk += src.SetParameter;
                    var flx = new Divergence_DerivativeSource_Flux(d, BcMap);
                    IBM_Op.EquationComponents["div"].Add(src);
                    IBM_Op.EquationComponents["div"].Add(flx);

                }

                if (((FSI_Control)this.Control).LevelSetMovement.ToLowerInvariant() == "fixed") {

                    var divPen = new BoSSS.Solution.NSECommon.Operator.Continuity.DivergenceAtIB(D, LsTrk, 1, delegate (double[] X, double time) {
                        throw new NotImplementedException("Currently not implemented for fixed motion");
                        //return new double[] { 0.0, 0.0 };
                    });
                    IBM_Op.EquationComponents["div"].Add(divPen);  // immersed boundary component
                } else {

                    var divPen = new BoSSS.Solution.NSECommon.Operator.Continuity.DivergenceAtIB(D, LsTrk, 1,
                       delegate (double[] X, double time) {

                           double[] result = new double[X.Length + 2];

                           foreach (Particle p in m_Particles) {
                               bool containsParticle;
                               if (m_Particles.Count == 1) {
                                   containsParticle = true;
                               } else { containsParticle = p.Contains(X); }


                               if (containsParticle) {
                                   result[0] = p.vel_P[0][0];
                                   result[1] = p.vel_P[0][1];
                                   result[2] = p.rot_P[0];
                                   result[3] = p.currentPos_P[0].L2Distance(X);
                                   return result;
                               }
                           }
                           return result;
                       });
                    IBM_Op.EquationComponents["div"].Add(divPen); // immersed boundary component 
                }
                //IBM_Op.EquationComponents["div"].Add(new PressureStabilization(1, 1.0 / this.Control.PhysicalParameters.mu_A));
            }
            IBM_Op.Commit();

            LevelSetHandling lsh;

            // create coupling
            // ------------------
            switch (((FSI_Control)this.Control).Timestepper_Mode) {
                case FSI_Control.TimesteppingMode.MovingMesh:
                    lsh = LevelSetHandling.Coupled_Once;
                    break;

                case FSI_Control.TimesteppingMode.Splitting:
                    lsh = LevelSetHandling.LieSplitting;
                    break;

                case FSI_Control.TimesteppingMode.None:
                    lsh = LevelSetHandling.None;
                    break;

                default:
                    throw new NotImplementedException();
            }



            // NSE or pure Stokes
            // ------------------
            SpatialOperatorType SpatialOp = SpatialOperatorType.LinearTimeDependent;
            if (this.Control.PhysicalParameters.IncludeConvection) {
                SpatialOp = SpatialOperatorType.Nonlinear;
            }


            // create timestepper
            // ------------------
            int bdfOrder;
            if (this.Control.Timestepper_Scheme == FSI_Control.TimesteppingScheme.CrankNicolson)
                bdfOrder = -1;
            //else if (this.Control.Timestepper_Scheme == IBM_Control.TimesteppingScheme.ExplicitEuler)
            //    bdfOrder = 0;
            else if (this.Control.Timestepper_Scheme == FSI_Control.TimesteppingScheme.ImplicitEuler)
                bdfOrder = 1;
            else if (this.Control.Timestepper_Scheme.ToString().StartsWith("BDF"))
                bdfOrder = Convert.ToInt32(this.Control.Timestepper_Scheme.ToString().Substring(3));
            else
                throw new NotImplementedException("todo");

            var MassMatrixShape = MassMatrixShapeandDependence.IsTimeDependent;

            //if (((FSI_Control)this.Control).LevelSetMovement.ToLowerInvariant() == "coupled" && ((FSI_Control)this.Control).includeTranslation == false)
            //    MassMatrixShape = MassMatrixShapeandDependence.IsNonIdentity;

            m_BDF_Timestepper = new XdgBDFTimestepping(
                ArrayTools.Cat(this.Velocity, this.Pressure),
                ArrayTools.Cat(this.ResidualMomentum, this.ResidualContinuity),
                LsTrk, true,
                DelComputeOperatorMatrix, DelUpdateLevelset,
                bdfOrder,
                lsh,
                MassMatrixShape,
                SpatialOp,
                MassScale,
                this.MultigridOperatorConfig, base.MultigridSequence,
                this.FluidSpecies, base.HMForder,
                this.Control.AdvancedDiscretizationOptions.CellAgglomerationThreshold, true);
            m_BDF_Timestepper.m_ResLogger = base.ResLogger;
            m_BDF_Timestepper.m_ResidualNames = ArrayTools.Cat(this.ResidualMomentum.Select(f => f.Identification), this.ResidualContinuity.Identification);
            m_BDF_Timestepper.Config_SolverConvergenceCriterion = this.Control.Solver_ConvergenceCriterion;
            m_BDF_Timestepper.Config_MaxIterations = this.Control.MaxSolverIterations;
            m_BDF_Timestepper.Config_MinIterations = this.Control.MinSolverIterations;
            m_BDF_Timestepper.SessionPath = SessionPath;
            m_BDF_Timestepper.Timestepper_Init = Solution.Timestepping.TimeStepperInit.SingleInit;

        }

        public override double DelUpdateLevelset(DGField[] CurrentState, double phystime, double dt, double UnderRelax, bool incremental) {

            /// FOR FIXED MOTION
            /// 
            switch (((FSI_Control)this.Control).LevelSetMovement.ToLowerInvariant()) {
                case "fixed":
                    ScalarFunction Posfunction = NonVectorizedScalarFunction.Vectorize(((FSI_Control)Control).MovementFunc, phystime);
                    newTransVelocity[0] = (((FSI_Control)this.Control).transVelocityFunc[0])(phystime);
                    newTransVelocity[1] = (((FSI_Control)this.Control).transVelocityFunc[1])(phystime);
                    LevSet.ProjectField(Posfunction);
                    LsTrk.UpdateTracker();
                    break;

                case "coupled":
                    //if (phystime == 0) {
                    //    oldPosition[0] = ((FSI_Control)this.Control).initialPos[0][0];
                    //    oldPosition[1] = ((FSI_Control)this.Control).initialPos[0][1];
                    //    newTransVelocity[0] = 0;
                    //    newTransVelocity[1] = 0;
                    //    oldTransVelocity[0] = 0;
                    //    oldTransVelocity[1] = 0;
                    //    TransVelocityN2[0] = 0;
                    //    TransVelocityN2[1] = 0;
                    //    TransVelocityN3[0] = 0;
                    //    TransVelocityN3[1] = 0;
                    //    TransVelocityN4[0] = 0;
                    //    TransVelocityN4[1] = 0;
                    //    force[0] = 0;
                    //    force[1] = 0;
                    //}



                    // Console.WriteLine("Particle Reynoldsnumber:   {0}", IBMMover.ComputeParticleRe(newTransVelocity, this.Control.particleRadius, ((FSI_Control)this.Control).particleRho));
                    //Func<double[], double, double> phiComplete;



                    //foreach (Particle p in m_Particles) {

                    //    p.UpdateParticlePosition(dt);


                    //    p.UpdateAngularVelocity(dt);
                    //    p.UpdateTransVelocity(dt);
                    //    //p.CleanHistory();
                    //    //phiComplete = phiComplete* p.phi_P;
                    //}


                    ////newPosition = IBMMover.MoveCircularParticle(dt, newTransVelocity, oldPosition);
                    ////TransVelocityN4 = TransVelocityN3;
                    ////TransVelocityN3 = TransVelocityN2;
                    ////TransVelocityN2 = oldTransVelocity;
                    ////oldTransVelocity = newTransVelocity;
                    ////oldPosition = newPosition;


                    //if (m_Particles.Count > 3)
                    //    throw new NotImplementedException("Currently the solver is only working for up to 2 particles");
                    //Func<double[], double, double> phiComplete;

                    //if (m_Particles.Count == 1) {
                    //    phiComplete = (X, t) => -1 * m_Particles[0].phi_P(X, t);
                    //} else if (m_Particles.Count == 2) { phiComplete = (X, t) => -1 * (m_Particles[0].phi_P(X, t) * m_Particles[1].phi_P(X, t)); } else {
                    //    phiComplete = (X, t) => -1 * (m_Particles[0].phi_P(X, t) * m_Particles[1].phi_P(X, t) * m_Particles[2].phi_P(X, t));
                    //}


                    //ScalarFunction function = NonVectorizedScalarFunction.Vectorize(phiComplete, phystime);
                    //LevSet.ProjectField(function);
                    //DGLevSet.Current.ProjectField(function);
                    //LsTrk.UpdateTracker(__NearRegionWith: 2);
                    UpdateLevelSetParticles(dt);
                    break;

                default:
                    throw new ApplicationException("unknown 'LevelSetMovement': " + ((FSI_Control)Control).LevelSetMovement);
            }

            return 0.0;
        }

        void UpdateLevelSetParticles(double dt) {
            foreach (Particle p in m_Particles) {

                p.UpdateParticlePosition(dt);

                Console.WriteLine("Current Velocites are:   " + p.vel_P[0][0] + "        " + p.vel_P[0][1] + "       " + p.rot_P[0]);
                p.UpdateAngularVelocity(dt);
                p.UpdateTransVelocity(dt, this.Control.PhysicalParameters.rho_A);
                //p.CleanHistory();
                //phiComplete = phiComplete* p.phi_P;
            }


            //newPosition = IBMMover.MoveCircularParticle(dt, newTransVelocity, oldPosition);
            //TransVelocityN4 = TransVelocityN3;
            //TransVelocityN3 = TransVelocityN2;
            //TransVelocityN2 = oldTransVelocity;
            //oldTransVelocity = newTransVelocity;
            //oldPosition = newPosition;


            if (m_Particles.Count > 5)
                throw new NotImplementedException("Currently the solver is only working for up to 2 particles");
            Func<double[], double, double> phiComplete = null;

            if (m_Particles.Count == 1) {
                phiComplete = (X, t) => 1 * m_Particles[0].phi_P(X, t);
            } else if (m_Particles.Count == 2) {
                phiComplete = (X, t) => -1 * (m_Particles[0].phi_P(X, t) * m_Particles[1].phi_P(X, t));
            } else if (m_Particles.Count == 3) {
                phiComplete = (X, t) => -1 * (m_Particles[0].phi_P(X, t) * m_Particles[1].phi_P(X, t) * m_Particles[2].phi_P(X, t));
            } else if (m_Particles.Count == 4) {
                phiComplete = (X, t) => -1 * (m_Particles[0].phi_P(X, t) * m_Particles[1].phi_P(X, t) * m_Particles[2].phi_P(X, t) * m_Particles[3].phi_P(X, t));
            } else if (m_Particles.Count == 5) {
                phiComplete = (X, t) => 1 * (m_Particles[0].phi_P(X, t) * m_Particles[1].phi_P(X, t) * (m_Particles[2].phi_P(X, t) * m_Particles[3].phi_P(X, t)) * m_Particles[4].phi_P(X, t));
            }



            ScalarFunction function = NonVectorizedScalarFunction.Vectorize(phiComplete, hack_phystime);
            LevSet.ProjectField(function);
            DGLevSet.Current.ProjectField(function);
            LsTrk.UpdateTracker(__NearRegionWith: 2);
        }

        /// <summary>
        /// Variables for FSI coupling
        /// </summary>
        double oldAngularVelocity, newAngularVelocity, MPIangularVelocity;
        double[] TransVelocityN4 = new double[2];
        double[] TransVelocityN3 = new double[2];
        double[] TransVelocityN2 = new double[2];
        double[] oldTransVelocity = new double[2];
        double[] newTransVelocity = new double[2];
        double[] oldPosition = new double[2];
        double[] newPosition = new double[2];
        //double[] force = new double[2];
        double[] oldforce = new double[2];
        //double torque = new double();
        //double oldtorque = new double();
        double[] MPItransVelocity = new double[2];
        double[] MPIpos = new double[2];
        double totalMomentumOld = 0;

        protected override double RunSolverOneStep(int TimestepInt, double phystime, double dt) {
            using (new FuncTrace()) {
                TimestepNumber TimestepNo = new TimestepNumber(TimestepInt, 0);
                int D = this.GridData.SpatialDimension;

                base.ResLogger.TimeStep = TimestepInt;

                hack_phystime = phystime;
                dt = base.GetFixedTimestep();

                if (((FSI_Control)this.Control).pureDryCollisions) {
                    UpdateLevelSetParticles(dt);
                } else {

                    if (triggerOnlyCollisionProcedure) {
                        UpdateLevelSetParticles(dt);
                        triggerOnlyCollisionProcedure = false;
                        if (phystime == 0) {
                            if ((base.MPIRank == 0) && (CurrentSessionInfo.ID != Guid.Empty)) {
                                Log_DragAndLift = base.DatabaseDriver.FsDriver.GetNewLog("PhysicalData", CurrentSessionInfo.ID);
                                string firstline = String.Format("{0}\t{1}\t{2}\t{3}\t{4}\t{5}\t{6}\t{7}\t{8}\t{9}", "#Timestep", "#Time", "P0_PosX", "P0_PosY", "P0_angle", "P0_VelX", "P0_VelY", "xPosition", "TotalKineticEnergy", "TotalMomentum");
                                Log_DragAndLift.WriteLine(firstline);
                            }
                        }
                        return dt;
                    } else {
                        m_BDF_Timestepper.Solve(phystime, dt, false);
                    }
                }


                this.ResLogger.NextTimestep(false);

                // L2 error against exact solution
                // ===============================
                //this.ComputeL2Error();

                //oldMassMatrix = MassFact.GetMassMatrix(CurrentSolution.Mapping, MassScale);

                #region Get Drag and Lift Coefficiant
                if (phystime == 0) {
                    if ((base.MPIRank == 0) && (CurrentSessionInfo.ID != Guid.Empty)) {
                        Log_DragAndLift = base.DatabaseDriver.FsDriver.GetNewLog("PhysicalData", CurrentSessionInfo.ID);
                        string firstline = String.Format("{0}\t{1}\t{2}\t{3}\t{4}\t{5}\t{6}\t{7}\t{8}\t{9}", "#Timestep", "#Time", "P0_PosX", "P0_PosY", "P0_angle", "P0_VelX", "P0_VelY", "xPosition", "TotalKineticEnergy", "TotalMomentum");
                        Log_DragAndLift.WriteLine(firstline);
                    }
                }

                foreach (Particle p in m_Particles) {
                    if (!((FSI_Control)this.Control).pureDryCollisions) {
                        p.UpdateForcesAndTorque(Velocity, Pressure, LsTrk, this.Control.PhysicalParameters.mu_A);
                    }
                    WallCollisionForces(p, LsTrk.GridDat.Cells.h_minGlobal);
                }

                double[] totalMomentum = new double[2] { 0, 0 };
                double[] totalKE = new double[3] { 0, 0, 0 };

                foreach (Particle p in m_Particles) {
                    totalMomentum[0] += p.mass_P * p.vel_P[0][0];
                    totalMomentum[1] += p.mass_P * p.vel_P[0][1];
                    totalKE[0] += 0.5 * p.mass_P * p.vel_P[0][0].Pow2();
                    totalKE[1] += 0.5 * p.mass_P * p.vel_P[0][1].Pow2();
                    totalKE[2] += 0.5 * p.MomentOfInertia_P * p.rot_P[0].Pow2();
                }

                Console.WriteLine("Total-Momentum in System:  " + Math.Sqrt(totalMomentum[0].Pow2() + totalMomentum[1].Pow2()));
                Console.WriteLine("Total-KineticEnergy in System:  " + (totalKE[0] + totalKE[1] + totalKE[2]));

                totalMomentumOld = Math.Sqrt(totalMomentum[0].Pow2() + totalMomentum[1].Pow2());

                if (m_Particles.Count > 1)
                    UpdateCollisionForces(m_Particles, LsTrk.GridDat.Cells.h_minGlobal);

                force = m_Particles[0].forces_P[0];
                torque = m_Particles[0].torque_P[0];


                MPItransVelocity = m_Particles[0].vel_P[0];
                MPIangularVelocity = m_Particles[0].rot_P[0];

                // It always gets quick and dirty before a conference
                //if (newTransVelocity[0].MPIMax() != 0) { MPItransVelocity[0] = newTransVelocity[0].MPIMax(); } else { MPItransVelocity[0] = newTransVelocity[0].MPIMin(); }
                //if (newTransVelocity[1].MPIMax() != 0) { MPItransVelocity[1] = newTransVelocity[1].MPIMax(); } else { MPItransVelocity[1] = newTransVelocity[1].MPIMin(); }
                //if (newAngularVelocity.MPIMax() != 0) { MPIangularVelocity = newAngularVelocity.MPIMax(); } else { MPIangularVelocity = newAngularVelocity.MPIMin(); }
                //if (newPosition[0].MPIMax() != 0) { MPIpos[0] = newPosition[0].MPIMax(); } else { MPIpos[0] = newPosition[0].MPIMin(); }
                //if (newPosition[1].MPIMax() != 0) { MPIpos[1] = newPosition[1].MPIMax(); } else { MPIpos[1] = newPosition[1].MPIMin(); }

                Console.WriteLine(newPosition[1].MPIMax());

                if ((base.MPIRank == 0) && (Log_DragAndLift != null)) {
                    double drag = force[0];
                    double lift = force[1];
                    //string line = String.Format("{0}\t{1}\t{2}\t{3}\t{4}\t{5}\t{6}\t{7}\t{8}\t{9}", TimestepNo, phystime, drag, lift, newTransVelocity[0], newTransVelocity[1], newAngularVelocity, newPosition[0], newPosition[1], IBMMover.ComputeParticleRe(newTransVelocity, this.Control.particleRadius, ((FSI_Control)this.Control).particleRho));
                    string line = String.Format("{0}\t{1}\t{2}\t{3}\t{4}\t{5}\t{6}\t{7}\t{8}\t{9}", TimestepNo, phystime, m_Particles[0].currentPos_P[0][0], m_Particles[0].currentPos_P[0][1], m_Particles[0].currentAng_P[0], m_Particles[0].vel_P[0][0], m_Particles[0].vel_P[0][1], 0.0, (totalKE[0] + totalKE[1] + totalKE[2]), Math.Sqrt(totalMomentum[0].Pow2() + totalMomentum[1].Pow2()));
                    Log_DragAndLift.WriteLine(line);
                    Log_DragAndLift.Flush();
                }


                // if ((newAngularVelocity - oldAngularVelocity) < 0) TerminationKey = true;

                oldAngularVelocity = newAngularVelocity;

                //newAngularVelocity = IBMMover.GetAngularVelocity(dt, oldAngularVelocity, Control.particleRadius, ((FSI_Control)this.Control).particleMass, torque, oldtorque, ((FSI_Control)Control).includeRotation);

                Console.WriteLine("Drag Force:   {0}", force[0]);
                Console.WriteLine("Lift Force:   {0}", force[1]);
                Console.WriteLine("Torqe:   {0}", torque);
                Console.WriteLine("Transl VelocityX:   {0}", MPItransVelocity[0]);
                Console.WriteLine("Transl VelocityY:   {0}", MPItransVelocity[1]);
                Console.WriteLine("Angular Velocity:   {0}", MPIangularVelocity);
                Console.WriteLine();


                // Save for NUnit Test
                base.QueryHandler.ValueQuery("C_Drag", 2 * force[0], true); // Only for Diameter 1 (TestCase NSE stationary)
                base.QueryHandler.ValueQuery("C_Lift", 2 * force[1], true); // Only for Diameter 1 (TestCase NSE stationary)
                base.QueryHandler.ValueQuery("Angular_Velocity", MPIangularVelocity, true); // (TestCase FlowRotationalCoupling)
                #endregion


                // Which Quadratur is beeing used? 

                //foreach (var Variant in this.LsTrk.m_QuadFactoryHelpers.Keys) {
                //    Console.WriteLine("Variant: " + Variant + " ################################################  ");

                //    foreach (int order in this.LsTrk.GetXQuadFactoryHelper(Variant).GetCachedSurfaceOrders(0))
                //        Console.WriteLine("    ---- used surface order :" + order);
                //    foreach (int order in this.LsTrk.GetXQuadFactoryHelper(Variant).GetCachedVolumeOrders(0))
                //        Console.WriteLine("    ---- used volume order :" + order);
                //}


                return dt;
            }
        }

        public override void PostRestart(double time, TimestepNumber timestep) {
            //var fsDriver = this.DatabaseDriver.FsDriver;
            //string pathToOldSessionDir = System.IO.Path.Combine(
            //    fsDriver.BasePath, "sessions", this.CurrentSessionInfo.RestartedFrom.ToString());
            //string pathToPhysicalData = System.IO.Path.Combine(pathToOldSessionDir,"PhysicalData.txt");
            //string[] records = File.ReadAllLines(pathToPhysicalData); 

            //string line1 = File.ReadLines(pathToPhysicalData).Skip(1).Take(1).First();
            //string line2 = File.ReadLines(pathToPhysicalData).Skip(2).Take(1).First();
            //string[] fields_line1 = line1.Split('\t');
            //string[] fields_line2 = line2.Split('\t');

            //Console.WriteLine("Line 1 " + fields_line1);

            //double dt = Convert.ToDouble(fields_line2[1]) - Convert.ToDouble(fields_line1[1]);

            //int idx_restartLine = Convert.ToInt32(time/dt + 1.0);
            //string restartLine = File.ReadLines(pathToPhysicalData).Skip(idx_restartLine-1).Take(1).First();
            //double[] values = Array.ConvertAll<string, double>(restartLine.Split('\t'), double.Parse);

            //if (time == values[1]+dt)
            //{
            //    Console.WriteLine("Restarting from time " + values[1]);
            //}

            //oldPosition[0] = values[7];
            //oldPosition[1] = values[8];
            //newTransVelocity[0] = values[4];
            //newTransVelocity[1] = values[5];
            //oldTransVelocity[0] = 0;
            //oldTransVelocity[1] = 0;
            //TransVelocityN2[0] = 0;
            //TransVelocityN2[1] = 0;
            //TransVelocityN3[0] = 0;
            //TransVelocityN3[1] = 0;
            //TransVelocityN4[0] = 0;
            //TransVelocityN4[1] = 0;
            //force[0] = values[2];
            //force[1] = values[3];

            //if ((base.MPIRank == 0) && (CurrentSessionInfo.ID != Guid.Empty))
            //{
            //    Log_DragAndLift = base.DatabaseDriver.FsDriver.GetNewLog("PhysicalData", CurrentSessionInfo.ID);
            //    string firstline = String.Format("{0}\t{1}\t{2}\t{3}\t{4}\t{5}\t{6}\t{7}\t{8}\t{9}", "#Timestep", "#Time", "DragForce", "LiftForce", "VelocityX", "VelocityY", "AngularVelocity", "xPosition", "yPosition", "ParticleRe");
            //    Log_DragAndLift.WriteLine(firstline);
            //    Log_DragAndLift.WriteLine(restartLine);
            //}


        }

        protected override void SetInitial() {
            base.SetInitial();

            // Setup particles
            m_Particles = ((FSI_Control)this.Control).Particles;

            // Setup Collision Model
            m_collisionModel = ((FSI_Control)this.Control).collisionModel;


            foreach (Particle p in m_Particles) {
                p.m_collidedWithParticle = new bool[m_Particles.Count];
                p.m_collidedWithWall = new bool[4];
                p.m_closeInterfacePointTo = new double[m_Particles.Count][];
            }


        }

        List<Particle> m_Particles;

        bool collision = false;

        bool triggerOnlyCollisionProcedure = true;

        /// <summary>
        /// Update collisionforces between two arbitrary particles and add them to forces acting on the corresponding particle
        /// </summary>
        /// <param name="particle0"></param>
        /// <param name="particle1"></param>
        public void UpdateCollisionForces(List<Particle> particles, double hmin) {

            if (particles.Count < 2)
                return;

            // Most of the code resulted from old one, should be simplified soon
            for (int i = 0; i < particles.Count; i++) {
                for (int j = i + 1; j < particles.Count; j++) {
                    var particle0 = particles[i];
                    var particle1 = particles[j];

                    var particle0CutCells = particle0.cutCells_P(LsTrk);
                    var particle1CutCells = particle1.cutCells_P(LsTrk);

                    var particleCutCellArray_P0 = particle0CutCells.ItemEnum.ToArray();
                    var neighborCellsArray_P0 = particle0CutCells.AllNeighbourCells().ItemEnum.ToArray();
                    var allCellsArray_P0 = particleCutCellArray_P0.Concat(neighborCellsArray_P0).ToArray();
                    var allCells_P0 = new CellMask(GridData, neighborCellsArray_P0);

                    var particleCutCellArray_P1 = particle1CutCells.ItemEnum.ToArray();
                    var neighborCellsArray_P1 = particle1CutCells.AllNeighbourCells().ItemEnum.ToArray();
                    var allCellsArray_P1 = particleCutCellArray_P1.Concat(neighborCellsArray_P1).ToArray();
                    var allCells_P1 = new CellMask(GridData, neighborCellsArray_P1);

                    double distance = 1E20;
                    double[] distanceVec = new double[Grid.SpatialDimension];

                    var interSecMask = allCells_P0.Intersect(allCells_P1);

                    var p0intersect = interSecMask.AllNeighbourCells().Intersect(particle0CutCells);
                    var p1intersect = interSecMask.AllNeighbourCells().Intersect(particle1CutCells);
                    
                    // If there is no element neighbour of both particle cut cells return
                    if (!interSecMask.IsEmpty) {

                        // All interface points at a specific subgrid containing all cut cells of one particle
                        var interfacePoints_P0 = BoSSS.Solution.XNSECommon.XNSEUtils.GetInterfacePoints(LsTrk, LevSet, new SubGrid(particle0CutCells));
                        var interfacePoints_P1 = BoSSS.Solution.XNSECommon.XNSEUtils.GetInterfacePoints(LsTrk, LevSet, new SubGrid(particle1CutCells));

                        var tempDistance = 0.0;
                        double[] tempPoint_P0 = new double[2] { 0.0, 0.0 };
                        double[] tempPoint_P1 = new double[2] { 0.0, 0.0 };

                        if (interfacePoints_P0 != null && interfacePoints_P1 !=null) {

                            for (int f = 0; f < interfacePoints_P0.NoOfRows; f++) {
                                for (int g = 0; g < interfacePoints_P1.NoOfRows; g++) {
                                    tempDistance = Math.Sqrt((interfacePoints_P0.GetRow(f)[0] - interfacePoints_P1.GetRow(g)[0]).Pow2() + (interfacePoints_P0.GetRow(f)[1] - interfacePoints_P1.GetRow(g)[1]).Pow2());
                                    if (tempDistance < distance) {
                                        //distanceVec.ClearEntries();
                                        //distanceVec.AccV(1, interfacePoints_P0.GetRow(f));
                                        distanceVec = interfacePoints_P0.GetRow(f).CloneAs();
                                        distanceVec.AccV(-1, interfacePoints_P1.GetRow(g));
                                        tempPoint_P0 = interfacePoints_P0.GetRow(f);
                                        tempPoint_P1 = interfacePoints_P1.GetRow(g);
                                        distance = tempDistance;
                                    }
                                }
                            }
                        }

                        double realDistance = distance;
                        bool ForceCollision = false;

                        // Important to get normal vector if distance is overlapping in the next timestep
                        if (realDistance <= 0.0) {
                            tempPoint_P0 = particle0.m_closeInterfacePointTo[m_Particles.IndexOf(particle1)];
                            tempPoint_P1 = particle1.m_closeInterfacePointTo[m_Particles.IndexOf(particle0)];
                            distanceVec = tempPoint_P0.CloneAs();
                            distanceVec.AccV(-1, tempPoint_P1);
                            ForceCollision = true;
                        }
                        particle0.m_closeInterfacePointTo[m_Particles.IndexOf(particle1)] = tempPoint_P0;
                        particle1.m_closeInterfacePointTo[m_Particles.IndexOf(particle0)] = tempPoint_P1;



                        double eps = hmin.Pow2();
                        double epsPrime = hmin;
                        //distanceVec.AccV(-1.0, particle1.currentPos_P[0]);
                        double threshold = 2.5 * hmin;

                        double[] collisionForce;

                        var massDifference = Math.Abs(this.Control.PhysicalParameters.rho_A - particle0.rho_P);

                        Console.WriteLine("realDistance: " + realDistance);
                        Console.WriteLine("Threshold: " + threshold);
                        Console.WriteLine("hmin: " + hmin);




                        //if ((realDistance <= threshold) && (realDistance >= (particle0.radius_P + particle1.radius_P))) {

                        //distanceVec.ScaleV((threshold - realDistance).Abs

                        // test of Modell 2

                        switch (m_collisionModel) {

                            case (FSI_Solver.FSI_Control.CollisionModel.RepulsiveForce_v1):
                                if ((realDistance <= threshold)) {
                                    distanceVec.ScaleV((threshold - realDistance).Pow2());
                                    distanceVec.ScaleV(1 / eps);

                                    Console.WriteLine("!!!!!!!!!!!!!!!!!!!!!!!!!!!!realDistance: " + realDistance);

                                    collisionForce = distanceVec;
                                    var collisionForceP1 = collisionForce.CloneAs();
                                    collisionForce.ScaleV(-100.0);
                                    collisionForceP1.ScaleV(-100.0);
                                    particle0.forces_P[0].AccV(-1, collisionForce);
                                    //particle0.torque_P[0] += 100 * (collisionForce[0] * (tempPoint_P0[0] - particle0.currentPos_P[0][0]) + collisionForce[1] * (tempPoint_P0[1] - particle0.currentPos_P[0][1]));
                                    particle1.forces_P[0].AccV(1, collisionForceP1);
                                    //particle1.torque_P[0] += -100 * (collisionForceP1[0] * (tempPoint_P1[0] - particle1.currentPos_P[0][0]) + collisionForceP1[1] * (tempPoint_P1[1] - particle1.currentPos_P[0][1]));
                                    Console.WriteLine("Collision information: Particles coming close, force " + collisionForce.L2Norm());
                                    Console.WriteLine("Collision information: Particles coming close, torque " + particle1.torque_P[0]);

                                    if (realDistance <= 1.5 * hmin) {
                                        Console.WriteLine("Entering overlapping loop....");
                                        triggerOnlyCollisionProcedure = true;
                                    }

                                }
                                break;



                            //case (FSI_Solver.FSI_Control.CollisionModel.RepulsiveForce_v2):
                            //distanceVec.ScaleV(1 / realDistance);

                            //double relVelocityX = particle0.vel_P[0][0] - particle1.vel_P[0][0];
                            //double relVelocityY = particle0.vel_P[0][1] - particle1.vel_P[0][1];

                            //double velocityAbs = Math.Sqrt(relVelocityX * relVelocityX + relVelocityY * relVelocityY);

                            //double muT = this.Control.PhysicalParameters.mu_A;

                            //distanceVec.ScaleV((muT * velocityAbs) / realDistance);

                            //collisionForce = distanceVec;
                            //var collisionForceP1 = collisionForce.CloneAs();
                            //collisionForce.ScaleV(-100.0);
                            //collisionForceP1.ScaleV(-100.0);
                            //particle0.forces_P[0].AccV(-1, collisionForce);
                            //particle0.torque_P[0] += 100 * (collisionForce[0] * (tempPoint_P0[0] - particle0.currentPos_P[0][0]) + collisionForce[1] * (tempPoint_P0[1] - particle0.currentPos_P[0][1]));
                            //particle1.forces_P[0].AccV(1, collisionForceP1);
                            //particle1.torque_P[0] += -100 * (collisionForceP1[0] * (tempPoint_P1[0] - particle1.currentPos_P[0][0]) + collisionForceP1[1] * (tempPoint_P1[1] - particle1.currentPos_P[0][1]));
                            //Console.WriteLine("Collision information: Particles coming close, force " + collisionForce.L2Norm());
                            //Console.WriteLine("Collision information: Particles coming close, torque " + particle1.torque_P[0]);
                            //break;

                            case (FSI_Solver.FSI_Control.CollisionModel.MomentumConservation):

                                if (((realDistance <= threshold) || ForceCollision) && !particle0.m_collidedWithParticle[m_Particles.IndexOf(particle1)] && !particle1.m_collidedWithParticle[m_Particles.IndexOf(particle0)]) {


                                    particle0.m_collidedWithParticle[m_Particles.IndexOf(particle1)] = true;
                                    particle1.m_collidedWithParticle[m_Particles.IndexOf(particle0)] = true;

                                    Console.WriteLine("!!!!!!!!!!!!!!!!!!!!!!!!!!!!realDistance: " + realDistance);

                                    //coefficient of restitution (e=0 pastic; e=1 elastic)
                                    double e = 1;

                                    //collision Nomal
                                    var normal = distanceVec.CloneAs();
                                    normal.ScaleV(1 / Math.Sqrt(distanceVec[0].Pow2() + distanceVec[1].Pow2()));

                                    double[] tangential = new double[] { -normal[1], normal[0] };


                                    //general definitions of normal and tangential components
                                    double collisionVn_P0 = particle0.vel_P[0][0] * normal[0] + particle0.vel_P[0][1] * normal[1];
                                    double collisionVt_P0 = particle0.vel_P[0][0] * tangential[0] + particle0.vel_P[0][1] * tangential[1];
                                    double collisionVn_P1 = particle1.vel_P[0][0] * normal[0] + particle1.vel_P[0][1] * normal[1];
                                    double collisionVt_P1 = particle1.vel_P[0][0] * tangential[0] + particle1.vel_P[0][1] * tangential[1];

                                    // exzentric collision
                                    // ----------------------------------------                                                                  
                                    tempPoint_P0.AccV(-1, particle0.currentPos_P[0]);
                                    double a0 = (tempPoint_P0[0] * tangential[0] + tempPoint_P0[1] * tangential[1]);
                                    tempPoint_P1.AccV(-1, particle1.currentPos_P[0]);
                                    double a1 = (tempPoint_P1[0] * tangential[0] + tempPoint_P1[1] * tangential[1]);

                                    // Fix for Sphere
                                    // ----------------------------------------  
                                    if (particle0.m_shape == Particle.ParticleShape.spherical)
                                        a0 = 0.0;
                                    if (particle1.m_shape == Particle.ParticleShape.spherical)
                                        a1 = 0.0;


                                    double Fx = (1 + e) * ((collisionVn_P0 - collisionVn_P1) / (1 / particle0.mass_P + 1 / particle1.mass_P + a0.Pow2() / particle0.MomentOfInertia_P + a1.Pow2() / particle1.MomentOfInertia_P));
                                    double Fxrot = (1 + e) * ((-a0 * particle0.rot_P[0] + a1 * particle1.rot_P[0]) / (1 / particle0.mass_P + 1 / particle1.mass_P + a0.Pow2() / particle0.MomentOfInertia_P + a1.Pow2() / particle1.MomentOfInertia_P));

                                    double tempCollisionVn_P0 = collisionVn_P0 - (Fx + Fxrot) / particle0.mass_P;
                                    double tempCollisionVn_P1 = collisionVn_P1 + (Fx + Fxrot) / particle1.mass_P;
                                    double tempCollisionVt_P0 = collisionVt_P0;
                                    double tempCollisionVt_P1 = collisionVt_P1;
                                    Console.WriteLine("a0:    " + a0 + "   Fx:    " + (-Fx) + "      Fxrot:    " + (-Fxrot));
                                    Console.WriteLine("a1:    " + a1 + "   Fx:    " + Fx + "      Fxrot:    " + Fxrot);
                                    particle0.rot_P[0] = particle0.rot_P[0] + a0 * (Fx + Fxrot) / particle0.MomentOfInertia_P;
                                    particle1.rot_P[0] = particle1.rot_P[0] - a1 * (Fx + Fxrot) / particle1.MomentOfInertia_P;
                                    // ----------------------------------------

                                    //double tempCollisionVn_P0 = collisionVn_P0 - Math.Sign(collisionVn_P0) * (Fx.Abs() + Fxrot.Abs()) / particle0.mass_P;
                                    //double tempCollisionVn_P1 = collisionVn_P0 - Math.Sign(collisionVn_P1) * (Fx.Abs() + Fxrot.Abs()) / particle1.mass_P;                                 
                                    //particle.rot_P[0] -= tempCollisionVn_P0/a0;
                                    //particle0.rot_P[0] -= a0 * Math.Sign(particle0.rot_P[0]) * (Fx + Fxrot) / particle0.MomentOfInertia_P;
                                    //particle1.rot_P[0] += a0 * Math.Sign(particle1.rot_P[0]) * (Fx + Fxrot) / particle1.MomentOfInertia_P;


                                    // zentric collision
                                    // ----------------------------------------
                                    //double tempCollisionVn_P0 = (particle0.mass_P * collisionVn_P0 + particle1.mass_P * collisionVn_P1 + e * particle1.mass_P * (collisionVn_P1 - collisionVn_P0)) / (particle0.mass_P + particle1.mass_P);
                                    //double tempCollisionVt_P0 = collisionVt_P0;
                                    //double tempCollisionVn_P1 = (particle0.mass_P * collisionVn_P0 + particle1.mass_P * collisionVn_P1 + e * particle0.mass_P * (collisionVn_P0 - collisionVn_P1)) / (particle0.mass_P + particle1.mass_P);
                                    //double tempCollisionVt_P1 = collisionVt_P1;
                                    // ----------------------------------------


                                    particle0.vel_P[0] = new double[] { normal[0] * tempCollisionVn_P0 + tempCollisionVt_P0 * tangential[0], normal[1] * tempCollisionVn_P0 + tempCollisionVt_P0 * tangential[1] };
                                    particle1.vel_P[0] = new double[] { normal[0] * tempCollisionVn_P1 + tempCollisionVt_P1 * tangential[0], normal[1] * tempCollisionVn_P1 + tempCollisionVt_P1 * tangential[1] };
                                    //collided = true;

                                    // exzentrischer stoß
                                    //double contactForce = (1 + e)*(particle0.vel_P[0][0] - particle0.radius_P * particle0.rot_P[0] - (particle1.vel_P[0][0] - particle1.radius_P * particle1.rot_P[0])) / (1/particle0.mass_P+1/particle1.mass_P+particle0.radius_P.Pow2()/particle0.MomentOfInertia_P+particle1.radius_P.Pow2()/particle1.MomentOfInertia_P);
                                    //particle0.vel_P[0][0] -= contactForce / particle0.mass_P;
                                    //particle0.rot_P[0] = particle0.rot_P[0];
                                    //particle0.rot_P[0] += particle0.radius_P * contactForce / particle0.MomentOfInertia_P;
                                    //particle1.vel_P[0][0] -= contactForce / particle1.mass_P;
                                    //particle1.rot_P[0] = particle1.rot_P[0];
                                    //particle1.rot_P[0] += particle1.radius_P * contactForce / particle1.MomentOfInertia_P;

                                    if ((realDistance <= 1.5 * hmin) /*|| ForceCollision*/) {
                                        Console.WriteLine("Entering overlapping loop....");
                                        triggerOnlyCollisionProcedure = true;
                                    }
                                }

                                if (realDistance > threshold && particle0.m_collidedWithParticle[m_Particles.IndexOf(particle1)] && particle1.m_collidedWithParticle[m_Particles.IndexOf(particle0)]) {
                                    particle0.m_collidedWithParticle[m_Particles.IndexOf(particle1)] = false;
                                    particle1.m_collidedWithParticle[m_Particles.IndexOf(particle0)] = false;
                                    particle0.m_closeInterfacePointTo[m_Particles.IndexOf(particle1)] = null;
                                    particle1.m_closeInterfacePointTo[m_Particles.IndexOf(particle0)] = null;
                                }

                                ForceCollision = false;
                                break;

                            case (FSI_Solver.FSI_Control.CollisionModel.MomentumConservation_NoCollisionBool):

                                if (((realDistance <= threshold) || ForceCollision) && !particle0.m_collidedWithParticle[m_Particles.IndexOf(particle1)] && !particle1.m_collidedWithParticle[m_Particles.IndexOf(particle0)]) {


                                    //particle0.m_collidedWithParticle[m_Particles.IndexOf(particle1)] = true;
                                    //particle1.m_collidedWithParticle[m_Particles.IndexOf(particle0)] = true;

                                    Console.WriteLine("!!!!!!!!!!!!!!!!!!!!!!!!!!!!realDistance: " + realDistance);

                                    //coefficient of restitution (e=0 pastic; e=1 elastic)
                                    double e = 1;

                                    //collision Nomal
                                    var normal = distanceVec.CloneAs();
                                    normal.ScaleV(1 / Math.Sqrt(distanceVec[0].Pow2() + distanceVec[1].Pow2()));

                                    double[] tangential = new double[] { -normal[1], normal[0] };


                                    //general definitions of normal and tangential components
                                    double collisionVn_P0 = particle0.vel_P[0][0] * normal[0] + particle0.vel_P[0][1] * normal[1];
                                    double collisionVt_P0 = particle0.vel_P[0][0] * tangential[0] + particle0.vel_P[0][1] * tangential[1];
                                    double collisionVn_P1 = particle1.vel_P[0][0] * normal[0] + particle1.vel_P[0][1] * normal[1];
                                    double collisionVt_P1 = particle1.vel_P[0][0] * tangential[0] + particle1.vel_P[0][1] * tangential[1];

                                    // exzentric collision
                                    // ----------------------------------------                                                                  
                                    tempPoint_P0.AccV(-1, particle0.currentPos_P[0]);
                                    double a0 = (tempPoint_P0[0] * tangential[0] + tempPoint_P0[1] * tangential[1]);
                                    tempPoint_P1.AccV(-1, particle1.currentPos_P[0]);
                                    double a1 = (tempPoint_P1[0] * tangential[0] + tempPoint_P1[1] * tangential[1]);

                                    // Fix for Sphere
                                    // ----------------------------------------  
                                    if (particle0.m_shape == Particle.ParticleShape.spherical)
                                        a0 = 0.0;
                                    if (particle1.m_shape == Particle.ParticleShape.spherical)
                                        a1 = 0.0;


                                    double Fx = (1 + e) * ((collisionVn_P0 - collisionVn_P1) / (1 / particle0.mass_P + 1 / particle1.mass_P + a0.Pow2() / particle0.MomentOfInertia_P + a1.Pow2() / particle1.MomentOfInertia_P));
                                    double Fxrot = (1 + e) * ((-a0 * particle0.rot_P[0] + a1 * particle1.rot_P[0]) / (1 / particle0.mass_P + 1 / particle1.mass_P + a0.Pow2() / particle0.MomentOfInertia_P + a1.Pow2() / particle1.MomentOfInertia_P));

                                    double tempCollisionVn_P0 = collisionVn_P0 - (Fx + Fxrot) / particle0.mass_P;
                                    double tempCollisionVn_P1 = collisionVn_P1 + (Fx + Fxrot) / particle1.mass_P;
                                    double tempCollisionVt_P0 = collisionVt_P0;
                                    double tempCollisionVt_P1 = collisionVt_P1;
                                    Console.WriteLine("a0:    " + a0 + "   Fx:    " + (-Fx) + "      Fxrot:    " + (-Fxrot));
                                    Console.WriteLine("a1:    " + a1 + "   Fx:    " + Fx + "      Fxrot:    " + Fxrot);
                                    particle0.rot_P[0] = particle0.rot_P[0] + a0 * (Fx + Fxrot) / particle0.MomentOfInertia_P;
                                    particle1.rot_P[0] = particle1.rot_P[0] - a1 * (Fx + Fxrot) / particle1.MomentOfInertia_P;
                                    // ----------------------------------------

                                    //double tempCollisionVn_P0 = collisionVn_P0 - Math.Sign(collisionVn_P0) * (Fx.Abs() + Fxrot.Abs()) / particle0.mass_P;
                                    //double tempCollisionVn_P1 = collisionVn_P0 - Math.Sign(collisionVn_P1) * (Fx.Abs() + Fxrot.Abs()) / particle1.mass_P;                                 
                                    //particle.rot_P[0] -= tempCollisionVn_P0/a0;
                                    //particle0.rot_P[0] -= a0 * Math.Sign(particle0.rot_P[0]) * (Fx + Fxrot) / particle0.MomentOfInertia_P;
                                    //particle1.rot_P[0] += a0 * Math.Sign(particle1.rot_P[0]) * (Fx + Fxrot) / particle1.MomentOfInertia_P;


                                    // zentric collision
                                    // ----------------------------------------
                                    //double tempCollisionVn_P0 = (particle0.mass_P * collisionVn_P0 + particle1.mass_P * collisionVn_P1 + e * particle1.mass_P * (collisionVn_P1 - collisionVn_P0)) / (particle0.mass_P + particle1.mass_P);
                                    //double tempCollisionVt_P0 = collisionVt_P0;
                                    //double tempCollisionVn_P1 = (particle0.mass_P * collisionVn_P0 + particle1.mass_P * collisionVn_P1 + e * particle0.mass_P * (collisionVn_P0 - collisionVn_P1)) / (particle0.mass_P + particle1.mass_P);
                                    //double tempCollisionVt_P1 = collisionVt_P1;
                                    // ----------------------------------------


                                    particle0.vel_P[0] = new double[] { normal[0] * tempCollisionVn_P0 + tempCollisionVt_P0 * tangential[0], normal[1] * tempCollisionVn_P0 + tempCollisionVt_P0 * tangential[1] };
                                    particle1.vel_P[0] = new double[] { normal[0] * tempCollisionVn_P1 + tempCollisionVt_P1 * tangential[0], normal[1] * tempCollisionVn_P1 + tempCollisionVt_P1 * tangential[1] };
                                    //collided = true;

                                    // exzentrischer stoß
                                    //double contactForce = (1 + e)*(particle0.vel_P[0][0] - particle0.radius_P * particle0.rot_P[0] - (particle1.vel_P[0][0] - particle1.radius_P * particle1.rot_P[0])) / (1/particle0.mass_P+1/particle1.mass_P+particle0.radius_P.Pow2()/particle0.MomentOfInertia_P+particle1.radius_P.Pow2()/particle1.MomentOfInertia_P);
                                    //particle0.vel_P[0][0] -= contactForce / particle0.mass_P;
                                    //particle0.rot_P[0] = particle0.rot_P[0];
                                    //particle0.rot_P[0] += particle0.radius_P * contactForce / particle0.MomentOfInertia_P;
                                    //particle1.vel_P[0][0] -= contactForce / particle1.mass_P;
                                    //particle1.rot_P[0] = particle1.rot_P[0];
                                    //particle1.rot_P[0] += particle1.radius_P * contactForce / particle1.MomentOfInertia_P;

                                    if ((realDistance <= 1.5 * hmin) /*|| ForceCollision*/) {
                                        Console.WriteLine("Entering overlapping loop....");
                                        triggerOnlyCollisionProcedure = true;
                                    }
                                }

                                if (realDistance > 1.5 * hmin && particle0.m_collidedWithParticle[m_Particles.IndexOf(particle1)] && particle1.m_collidedWithParticle[m_Particles.IndexOf(particle0)]) {
                                    particle0.m_collidedWithParticle[m_Particles.IndexOf(particle1)] = false;
                                    particle1.m_collidedWithParticle[m_Particles.IndexOf(particle0)] = false;
                                    particle0.m_closeInterfacePointTo[m_Particles.IndexOf(particle1)] = null;
                                    particle1.m_closeInterfacePointTo[m_Particles.IndexOf(particle0)] = null;
                                }

                                ForceCollision = false;
                                break;

                            case (FSI_Solver.FSI_Control.CollisionModel.MomentumConservation_ModifiedCollisionBool):

                                if (((realDistance <= threshold) || ForceCollision) && !particle0.m_collidedWithParticle[m_Particles.IndexOf(particle1)] && !particle1.m_collidedWithParticle[m_Particles.IndexOf(particle0)]) {


                                    particle0.m_collidedWithParticle[m_Particles.IndexOf(particle1)] = true;
                                    particle1.m_collidedWithParticle[m_Particles.IndexOf(particle0)] = true;

                                    Console.WriteLine("!!!!!!!!!!!!!!!!!!!!!!!!!!!!realDistance: " + realDistance);

                                    //coefficient of restitution (e=0 pastic; e=1 elastic)
                                    double e = 1;

                                    //collision Nomal
                                    var normal = distanceVec.CloneAs();
                                    normal.ScaleV(1 / Math.Sqrt(distanceVec[0].Pow2() + distanceVec[1].Pow2()));

                                    double[] tangential = new double[] { -normal[1], normal[0] };


                                    //general definitions of normal and tangential components
                                    double collisionVn_P0 = particle0.vel_P[0][0] * normal[0] + particle0.vel_P[0][1] * normal[1];
                                    double collisionVt_P0 = particle0.vel_P[0][0] * tangential[0] + particle0.vel_P[0][1] * tangential[1];
                                    double collisionVn_P1 = particle1.vel_P[0][0] * normal[0] + particle1.vel_P[0][1] * normal[1];
                                    double collisionVt_P1 = particle1.vel_P[0][0] * tangential[0] + particle1.vel_P[0][1] * tangential[1];

                                    // exzentric collision
                                    // ----------------------------------------                                                                  
                                    tempPoint_P0.AccV(-1, particle0.currentPos_P[0]);
                                    double a0 = (tempPoint_P0[0] * tangential[0] + tempPoint_P0[1] * tangential[1]);
                                    tempPoint_P1.AccV(-1, particle1.currentPos_P[0]);
                                    double a1 = (tempPoint_P1[0] * tangential[0] + tempPoint_P1[1] * tangential[1]);

                                    // Fix for Sphere
                                    // ----------------------------------------  
                                    if (particle0.m_shape == Particle.ParticleShape.spherical)
                                        a0 = 0.0;
                                    if (particle1.m_shape == Particle.ParticleShape.spherical)
                                        a1 = 0.0;


                                    double Fx = (1 + e) * ((collisionVn_P0 - collisionVn_P1) / (1 / particle0.mass_P + 1 / particle1.mass_P + a0.Pow2() / particle0.MomentOfInertia_P + a1.Pow2() / particle1.MomentOfInertia_P));
                                    double Fxrot = (1 + e) * ((-a0 * particle0.rot_P[0] + a1 * particle1.rot_P[0]) / (1 / particle0.mass_P + 1 / particle1.mass_P + a0.Pow2() / particle0.MomentOfInertia_P + a1.Pow2() / particle1.MomentOfInertia_P));

                                    double tempCollisionVn_P0 = collisionVn_P0 - (Fx + Fxrot) / particle0.mass_P;
                                    double tempCollisionVn_P1 = collisionVn_P1 + (Fx + Fxrot) / particle1.mass_P;
                                    double tempCollisionVt_P0 = collisionVt_P0;
                                    double tempCollisionVt_P1 = collisionVt_P1;
                                    Console.WriteLine("a0:    " + a0 + "   Fx:    " + (-Fx) + "      Fxrot:    " + (-Fxrot));
                                    Console.WriteLine("a1:    " + a1 + "   Fx:    " + Fx + "      Fxrot:    " + Fxrot);
                                    particle0.rot_P[0] = particle0.rot_P[0] + a0 * (Fx + Fxrot) / particle0.MomentOfInertia_P;
                                    particle1.rot_P[0] = particle1.rot_P[0] - a1 * (Fx + Fxrot) / particle1.MomentOfInertia_P;
                                    // ----------------------------------------

                                    //double tempCollisionVn_P0 = collisionVn_P0 - Math.Sign(collisionVn_P0) * (Fx.Abs() + Fxrot.Abs()) / particle0.mass_P;
                                    //double tempCollisionVn_P1 = collisionVn_P0 - Math.Sign(collisionVn_P1) * (Fx.Abs() + Fxrot.Abs()) / particle1.mass_P;                                 
                                    //particle.rot_P[0] -= tempCollisionVn_P0/a0;
                                    //particle0.rot_P[0] -= a0 * Math.Sign(particle0.rot_P[0]) * (Fx + Fxrot) / particle0.MomentOfInertia_P;
                                    //particle1.rot_P[0] += a0 * Math.Sign(particle1.rot_P[0]) * (Fx + Fxrot) / particle1.MomentOfInertia_P;


                                    // zentric collision
                                    // ----------------------------------------
                                    //double tempCollisionVn_P0 = (particle0.mass_P * collisionVn_P0 + particle1.mass_P * collisionVn_P1 + e * particle1.mass_P * (collisionVn_P1 - collisionVn_P0)) / (particle0.mass_P + particle1.mass_P);
                                    //double tempCollisionVt_P0 = collisionVt_P0;
                                    //double tempCollisionVn_P1 = (particle0.mass_P * collisionVn_P0 + particle1.mass_P * collisionVn_P1 + e * particle0.mass_P * (collisionVn_P0 - collisionVn_P1)) / (particle0.mass_P + particle1.mass_P);
                                    //double tempCollisionVt_P1 = collisionVt_P1;
                                    // ----------------------------------------


                                    particle0.vel_P[0] = new double[] { normal[0] * tempCollisionVn_P0 + tempCollisionVt_P0 * tangential[0], normal[1] * tempCollisionVn_P0 + tempCollisionVt_P0 * tangential[1] };
                                    particle1.vel_P[0] = new double[] { normal[0] * tempCollisionVn_P1 + tempCollisionVt_P1 * tangential[0], normal[1] * tempCollisionVn_P1 + tempCollisionVt_P1 * tangential[1] };
                                    //collided = true;

                                    // exzentrischer stoß
                                    //double contactForce = (1 + e)*(particle0.vel_P[0][0] - particle0.radius_P * particle0.rot_P[0] - (particle1.vel_P[0][0] - particle1.radius_P * particle1.rot_P[0])) / (1/particle0.mass_P+1/particle1.mass_P+particle0.radius_P.Pow2()/particle0.MomentOfInertia_P+particle1.radius_P.Pow2()/particle1.MomentOfInertia_P);
                                    //particle0.vel_P[0][0] -= contactForce / particle0.mass_P;
                                    //particle0.rot_P[0] = particle0.rot_P[0];
                                    //particle0.rot_P[0] += particle0.radius_P * contactForce / particle0.MomentOfInertia_P;
                                    //particle1.vel_P[0][0] -= contactForce / particle1.mass_P;
                                    //particle1.rot_P[0] = particle1.rot_P[0];
                                    //particle1.rot_P[0] += particle1.radius_P * contactForce / particle1.MomentOfInertia_P;

                                    if ((realDistance <= 1.5 * hmin) /*|| ForceCollision*/) {
                                        Console.WriteLine("Entering overlapping loop....");
                                        triggerOnlyCollisionProcedure = true;
                                    }
                                }

                                if (realDistance > 1.5 * hmin && particle0.m_collidedWithParticle[m_Particles.IndexOf(particle1)] && particle1.m_collidedWithParticle[m_Particles.IndexOf(particle0)]) {
                                    particle0.m_collidedWithParticle[m_Particles.IndexOf(particle1)] = false;
                                    particle1.m_collidedWithParticle[m_Particles.IndexOf(particle0)] = false;
                                    particle0.m_closeInterfacePointTo[m_Particles.IndexOf(particle1)] = null;
                                    particle1.m_closeInterfacePointTo[m_Particles.IndexOf(particle0)] = null;
                                }

                                ForceCollision = false;
                                break;

                            default:
                                throw new NotImplementedException("Collision model not available");
                        }
                    }
                }
            }
        }



        bool collided = false;

        private FSI_Solver.FSI_Control.CollisionModel m_collisionModel;

        /// <summary>
        /// Calculation of collision forces between particle and wall
        /// </summary>
        /// <param name="particles"></param>
        /// <param name="hmin"></param>
        public void WallCollisionForces(Particle particle, double hmin) {

            var particleCutCells = particle.cutCells_P(LsTrk);

            var particleCutCellArray = particleCutCells.ItemEnum.ToArray();
            var neighborCellsArray = particleCutCells.AllNeighbourCells().ItemEnum.ToArray();
            var allCellsArray = particleCutCellArray.Concat(neighborCellsArray).ToArray();
            var allCells = new CellMask(GridData, neighborCellsArray);

            collision = false;

            double distance = double.MaxValue;
            double[] distanceVec = new double[Grid.SpatialDimension];

            // All interface points at a specific subgrid containing all cut cells of one particle
            MultidimensionalArray interfacePoints = null;

            Console.WriteLine("ParticleCutCellCount:   " + particleCutCells.Count());

            var trafo = GridData.iGeomEdges.Edge2CellTrafos;

            SubGrid allCellsGrid = new SubGrid(allCells);

            double[] tempPoint = new double[2] { 0.0, 0.0 };

            foreach (int iEdge in allCellsGrid.BoundaryEdgesMask.ItemEnum) {

                // Collision forces have to act
                if (GridData.iGeomEdges.IsEdgeBoundaryEdge(iEdge)) {

                    if (interfacePoints == null)
                        interfacePoints = BoSSS.Solution.XNSECommon.XNSEUtils.GetInterfacePoints(LsTrk, LevSet, new SubGrid(particleCutCells));

                    collision = true;
                    var jCell = GridData.iGeomEdges.CellIndices[iEdge, 0];
                    int iKref = GridData.iGeomEdges.GetRefElementIndex(jCell);
                    //int iKref = GridData.Cells.GetRefElementIndex(jCell);
                    NodeSet[] refNodes = GridData.iGeomEdges.EdgeRefElements.Select(Kref2 => Kref2.GetQuadratureRule(5 * 2).Nodes).ToArray();
                    NodeSet Nodes = refNodes.ElementAt(iKref);

                    //SubGrid oneCellSubGrid = new SubGrid(new CellMask(GridData, new int[] { jCell + 1 }));
                    //SubGrid _oneCellSubGrid = new SubGrid(new CellMask(GridData, Chunk.GetSingleElementChunk(jCell)));
                    //Debug.Assert(oneCellSubGrid.VolumeMask.NoOfItemsLocally == 1);
                    //Debug.Assert(oneCellSubGrid.VolumeMask.ItemEnum.First() == jCell);
                    //Debug.Assert(_oneCellSubGrid.VolumeMask.NoOfItemsLocally == 1);
                    //Debug.Assert(_oneCellSubGrid.VolumeMask.ItemEnum.First() == jCell);

                    //var allEdges = oneCellSubGrid.BoundaryEdgesMask;



                    //Debug.Assert(GridData.Edges.CellIndices[iEdge, 0] == jCell);
                    var trafoIdx = GridData.iGeomEdges.Edge2CellTrafoIndex[iEdge, 0];
                    var transFormed = trafo[trafoIdx].Transform(Nodes);
                    var newVertices = transFormed.CloneAs();
                    GridData.TransformLocal2Global(transFormed, newVertices, jCell);
                    var tempDistance = 0.0;

                    for (int i = 0; i < interfacePoints.NoOfRows; i++) {
                        for (int j = 0; j < newVertices.NoOfRows; j++) {
                            tempDistance = Math.Sqrt((interfacePoints.GetRow(i)[0] - newVertices.GetRow(j)[0]).Pow2() + (interfacePoints.GetRow(i)[1] - newVertices.GetRow(j)[1]).Pow2());
                            if (tempDistance < distance) {
                                tempPoint = interfacePoints.GetRow(i);
                                distanceVec = interfacePoints.GetRow(i).CloneAs();
                                distanceVec.AccV(-1, newVertices.GetRow(j));
                                distance = tempDistance;
                            }

                        }


                    }
                }
            }

            double realDistance = distance;

            if (collision == false)
                return;


            Console.WriteLine("Closes Distance to wall is: " + distance);

            //GridData.Cells.ClosestPointInCell()
            //double[] distanceVec = particle0.currentPos_P[0].CloneAs();
            //double eps = hmin.Pow2() / 2; // Turek paper
            //double epsPrime = hmin / 2; // Turek paper
            double eps = hmin.Pow2() / 2; // Turek paper
            double epsPrime = hmin / 2; // Turek paper
                                        //distanceVec.AccV(-1.0, particle.currentPos_P[0]);
                                        //double distance = Math.Sqrt(distanceVec[0] * distanceVec[0] + distanceVec[1] * distanceVec[1]);
            double threshold = 2 * hmin;


            double[] collisionForce;

            //Console.WriteLine("Distance to wall: " + distance);
            //Console.WriteLine("Threshold: " + threshold);

            //if (distance < (2*hmin)) {
            //    //distanceVec.ScaleV((threshold - distance).Pow2());
            //    distanceVec.ScaleV(1/hmin);
            //    //Console.WriteLine("Scaling factor:   " + (2 * hmin / distance));
            //    collisionForce = distanceVec;
            //    collisionForce[0] = collisionForce[0];// .ScaleV(-981 * massDifference);
            //    collisionForce[1] = collisionForce[1] * 981 * massDifference;
            //    particle.forces_P[0].AccV(1, collisionForce);
            //    Console.WriteLine("Collision information: Wall test model, force " + collisionForce.L2Norm());
            //    return;
            //}



            //if (distance > threshold) {
            //    Console.WriteLine("Collision information: No collision");
            //    return;
            //}



            switch (m_collisionModel) {

                case (FSI_Solver.FSI_Control.CollisionModel.RepulsiveForce_v1):
                    if ((realDistance <= threshold) && (realDistance > 1.5 * hmin)) {
                        // Modell 1
                        distanceVec.ScaleV(1 / eps);
                        distanceVec.ScaleV(((threshold - realDistance).Pow2()));


                        collisionForce = distanceVec;
                        collisionForce.ScaleV(100.0);
                        particle.forces_P[0].AccV(1, collisionForce);
                        particle.torque_P[0] -= 100 * (collisionForce[0] * (tempPoint[0] - particle.currentPos_P[0][0]) + collisionForce[1] * (tempPoint[1] - particle.currentPos_P[0][1]));
                        Console.WriteLine("Collision information: Wall coming close, force X " + collisionForce[0]);
                        Console.WriteLine("Collision information: Wall coming close, force Y " + collisionForce[1]);

                        return;
                    }


                    if (realDistance <= (1.5 * hmin)) {

                        distanceVec.ScaleV((threshold - realDistance).Abs());
                        distanceVec.ScaleV(1 / epsPrime);
                        collisionForce = distanceVec;
                        //collisionForce[0] = collisionForce[0];// .ScaleV(-981 * massDifference);
                        //collisionForce[1] = collisionForce[1];
                        collisionForce.ScaleV(100.0);
                        particle.forces_P[0].AccV(1, collisionForce);
                        Console.WriteLine("Collision information: Wall overlapping, force X " + collisionForce[0]);
                        Console.WriteLine("Collision information: Wall overlapping, force Y " + collisionForce[1]);

                        if (realDistance <= 1.5 * hmin) {
                            Console.WriteLine("Entering wall overlapping loop....");
                            triggerOnlyCollisionProcedure = true;
                            //double dt = GetFixedTimestep();
                            //hack_phystime += dt;
                            //particle0.UpdateTransVelocity(dt);
                            //particle1.UpdateTransVelocity(dt);
                            //particle0.UpdateParticlePosition(dt);
                            //particle1.UpdateParticlePosition(dt);
                            //UpdateLevelSetParticles(dt);
                            //UpdateCollisionForces(particles, hmin);
                        }
                        return;
                    }
                    break;
                case (FSI_Solver.FSI_Control.CollisionModel.MomentumConservation_NoCollisionBool):
                case (FSI_Solver.FSI_Control.CollisionModel.MomentumConservation_ModifiedCollisionBool):
                case (FSI_Solver.FSI_Control.CollisionModel.MomentumConservation):
                    // ONLY FOR DISKS AT THE MOMENT
                    if (realDistance <= (threshold) && !particle.m_collidedWithWall[0]) {

                        particle.m_collidedWithWall[0] = true;
                        //coefficient of restitution (e=0 pastic; e=1 elastic)
                        double e = 1;

                        //collision Nomal
                        var normal = distanceVec.CloneAs();
                        normal.ScaleV(1 / Math.Sqrt(distanceVec[0].Pow2() + distanceVec[1].Pow2()));
                        double[] tangential = new double[] { -normal[1], normal[0] };


                        double collisionVn_P0 = particle.vel_P[0][0] * normal[0] + particle.vel_P[0][1] * normal[1];
                        double collisionVt_P0 = particle.vel_P[0][0] * tangential[0] + particle.vel_P[0][1] * tangential[1];


                        // exzentric collision
                        // ----------------------------------------
                        tempPoint.AccV(-1, particle.currentPos_P[0]);
                        double a0 = (tempPoint[0] * tangential[0] + tempPoint[1] * tangential[1]);

                        if (particle.m_shape == Particle.ParticleShape.spherical)
                            a0 = 0.0;


                        double Fx = (1 + e) * (collisionVn_P0) / (1 / particle.mass_P + a0.Pow2() / particle.MomentOfInertia_P);
                        double Fxrot = (1 + e) * (-a0 * particle.rot_P[0]) / (1 / particle.mass_P + a0.Pow2() / particle.MomentOfInertia_P);

                        double tempCollisionVn_P0 = collisionVn_P0 - (Fx + Fxrot) / particle.mass_P;
                        double tempCollisionVt_P0 = collisionVt_P0;
                        //particle.rot_P[0] -= tempCollisionVn_P0/a0;
                        particle.rot_P[0] = particle.rot_P[0] + a0 * (Fx + Fxrot) / particle.MomentOfInertia_P;
                        // ----------------------------------------



                        // zentrischer Stoß
                        //double tempCollisionVn_P0 = -e * collisionVn_P0;
                        //double tempCollisionVt_P0 = collisionVt_P0;



                        particle.vel_P[0] = new double[] { normal[0] * tempCollisionVn_P0 + tempCollisionVt_P0 * tangential[0], normal[1] * tempCollisionVn_P0 + tempCollisionVt_P0 * tangential[1] };


                        //collided = true;
                        if ((realDistance <= 1.5 * hmin) /*|| ForceCollision*/) {
                            Console.WriteLine("Entering overlapping loop....");
                            triggerOnlyCollisionProcedure = true;
                        }
                    }
                    if (realDistance > threshold && particle.m_collidedWithWall[0]) {
                        Console.WriteLine("Reset Wall");
                        particle.m_collidedWithWall[0] = false;
                    }
                    break;

                default:
                    throw new NotImplementedException("Collision model not available");
            }



        }


    }

    //throw new ApplicationException(realDistance);

}




