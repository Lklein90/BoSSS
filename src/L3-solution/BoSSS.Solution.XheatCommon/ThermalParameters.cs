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
using System.Runtime.Serialization;
using System.Linq;
using System.Text;

using BoSSS.Foundation.Grid;
using BoSSS.Foundation.Grid.Classic;
using BoSSS.Solution.Control;
using BoSSS.Solution.NSECommon;
using BoSSS.Solution.Utils;

namespace BoSSS.Solution.XheatCommon {


    [DataContract]
    [Serializable]
    public class ThermalParameters : ICloneable {

        /// <summary>
        /// density of fluid A
        /// </summary>
        [DataMember]
        public double rho_A;

        /// <summary>
        /// density of fluid B
        /// </summary>
        [DataMember]
        public double rho_B;


        /// <summary>
        /// heat capacity of fluid A
        /// </summary>
        [DataMember]
        public double c_A;

        /// <summary>
        /// heat capacity of fluid B
        /// </summary>
        [DataMember]
        public double c_B;


        /// <summary>
        /// thermal conductivity of fluid A
        /// </summary>
        [DataMember]
        public double k_A;

        /// <summary>
        /// thermal conductivity of fluid B
        /// </summary>
        [DataMember]
        public double k_B;


        /// <summary>
        /// is the interface a material one or is it non-material?
        /// </summary>
        [DataMember]
        public bool Material;


        /// <summary>
        /// clone
        /// </summary>
        public object Clone() {
            var cl = new ThermalParameters();
            cl.rho_A = this.rho_A;
            cl.rho_B = this.rho_B;
            cl.c_A = this.c_A;
            cl.c_B = this.c_B;
            cl.k_A = this.k_A;
            cl.k_B = this.k_B;
            cl.Material = this.Material;
            return cl;
        }


    }
}
