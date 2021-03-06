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

using ilPSP;
using System;
using System.Collections.Generic;
using System.Linq;
using BoSSS.Solution.NSECommon;
using BoSSS.Platform;
using BoSSS.Solution.Control;
using BoSSS.Foundation.Grid;
using System.Diagnostics;
using BoSSS.Solution.Multigrid;
using ilPSP.Utils;
using BoSSS.Platform.Utils.Geom;
using BoSSS.Foundation.IO;
using BoSSS.Foundation.Grid.Classic;

namespace BoSSS.Application.FSI_Solver {
     public class HardcodedTestExamples {
     

        public static FSI_Control ParticleInShearFlow(string _DbPath = null, int k = 2, double VelXBase = 0.0)
        {
            FSI_Control C = new FSI_Control();

            const double BaseSize = 1.0;

            // basic database options
            // ======================

            C.savetodb = false;
            C.ProjectName = "ShearFlow_Test";
            C.ProjectDescription = "ShearFlow";
            C.Tags.Add("with immersed boundary method");

            // DG degrees
            // ==========

            C.FieldOptions.Add("VelocityX", new FieldOpts()
            {
                Degree = k,
                SaveToDB = FieldOpts.SaveToDBOpt.TRUE
            });
            C.FieldOptions.Add("VelocityY", new FieldOpts()
            {
                Degree = k,
                SaveToDB = FieldOpts.SaveToDBOpt.TRUE
            });
            C.FieldOptions.Add("Pressure", new FieldOpts()
            {
                Degree = k - 1,
                SaveToDB = FieldOpts.SaveToDBOpt.TRUE
            });
            C.FieldOptions.Add("PhiDG", new FieldOpts()
            {
                Degree = 2,
                SaveToDB = FieldOpts.SaveToDBOpt.TRUE
            });
            C.FieldOptions.Add("Phi", new FieldOpts()
            {
                Degree = 2,
                SaveToDB = FieldOpts.SaveToDBOpt.TRUE
            });

            // grid and boundary conditions
            // ============================

            C.GridFunc = delegate
            {
                double[] Xnodes = GenericBlas.Linspace(-2 * BaseSize, 2 * BaseSize, 21);
                double[] Ynodes = GenericBlas.Linspace(-3 * BaseSize, 3 * BaseSize, 31);
                var grd = Grid2D.Cartesian2DGrid(Xnodes, Ynodes, periodicX: false, periodicY: true);

                grd.EdgeTagNames.Add(1, "Velocity_Inlet_left");
                grd.EdgeTagNames.Add(2, "Velocity_Inlet_right");


                grd.DefineEdgeTags(delegate (double[] X)
                {
                    byte et = 0;
                    if (Math.Abs(X[0] - (-2 * BaseSize)) <= 1.0e-8)
                        et = 1;
                    if (Math.Abs(X[0] + (-2 * BaseSize)) <= 1.0e-8)
                        et = 2;

                    Debug.Assert(et != 0);
                    return et;
                });

                return grd;
            };




            C.AddBoundaryValue("Velocity_Inlet_left", "VelocityY", X => 0.02);
            C.AddBoundaryValue("Velocity_Inlet_right", "VelocityY", X => -0.02);

            // Boundary values for level-set
            //C.BoundaryFunc = new Func<double, double>[] { (t) => 0.1 * 2 * Math.PI * -Math.Sin(Math.PI * 2 * 1 * t), (t) =>  0};
            //C.BoundaryFunc = new Func<double, double>[] { (t) => 0, (t) => 0 };

            // Initial Values
            // ==============
            // Coupling Properties
            C.LevelSetMovement = "coupled";
            C.includeTranslation = false;
            C.includeRotation = true;

            // Particle Properties
            C.Particles = new List<Particle>();


            C.Particles.Add(new Particle(2, 4, new double[] { 0.0, 0.0 }) {
                radius_P = 0.4,
                rho_P = 1
            });
            Func<double[], double, double> phiComplete = (X, t) => 1;


            phiComplete = (X, t) => C.Particles[0].phi_P(X, t);

            //Func<double[], double, double> phi = (X, t) => -(X[0] - t+X[1]);
            //C.MovementFunc = phi;

            C.InitialValues_Evaluators.Add("Phi", X => phiComplete(X, 0));
            //C.InitialValues.Add("VelocityX#B", X => 1);
            C.InitialValues_Evaluators.Add("VelocityX", X => 0);
            C.InitialValues_Evaluators.Add("VelocityY", X => 0);
            //C.InitialValues.Add("Phi", X => -1);
            //C.InitialValues.Add("Phi", X => (X[0] - 0.41));

            // For restart
            //C.RestartInfo = new Tuple<Guid, TimestepNumber>(new Guid("fec14187-4e12-43b6-af1e-e9d535c78668"), -1);


            // Physical Parameters
            // ===================

            C.PhysicalParameters.rho_A = 1;
            C.PhysicalParameters.mu_A = 0.25;
            C.PhysicalParameters.IncludeConvection = true;


            // misc. solver options
            // ====================
            C.AdvancedDiscretizationOptions.PenaltySafety = 1;
            C.AdvancedDiscretizationOptions.CellAgglomerationThreshold = 0.1;
            C.LevelSetSmoothing = false;
            C.MaxSolverIterations = 100;
            C.MinSolverIterations = 1;
            C.NoOfMultigridLevels = 1;

            // Timestepping
            // ============

            C.Timestepper_Scheme = FSI_Control.TimesteppingScheme.BDF2;
            double dt = 0.1;
            C.dtFixed = dt;
            C.dtMax = dt;
            C.dtMin = dt;
            C.Endtime = 120;
            C.NoOfTimesteps = 100;

            // haben fertig...
            // ===============

            return C;
        }
    }
}
