using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    public class Option : Instrument
    {
        public Proto.OptionType OptionType { get; set; }
        public Proto.ExerciseType ExerciseType { get; set; }
        public Proto.SettlementType SettlementType { get; set; }
        public double Strike { get; set; }
    }
}
