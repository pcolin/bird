using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    class PositionManager
    {
        public void Add(IList<Proto.Position> positions)
        {
            lock (this.positions)
            {
                foreach (var p in positions)
                {
                    this.positions[p.Instrument] = p;
                }
            }
        }

        public int GetPosition(string instrument)
        {
            lock (this.positions)
            {
                Proto.Position p = null;
                if (positions.TryGetValue(instrument, out p))
                {
                    return p.TotalLong - p.TotalShort;
                }
            }
            return 0;
        }

        public bool GetPosition(string instrument, out Proto.Position p)
        {
            lock (this.positions)
            {
                return positions.TryGetValue(instrument, out p);
            }
        }

        Dictionary<string, Proto.Position> positions = new Dictionary<string,Proto.Position>();
    }
}
