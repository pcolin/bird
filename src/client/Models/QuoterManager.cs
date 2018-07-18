using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Models
{
    class QuoterManager
    {
        public void OnProtoMessage(Proto.QuoterRep rep)
        {
            lock (this.quoters)
            {
                foreach (var p in rep.Quoters)
                {
                    quoters.Add(p.Name, p);
                }
            }
        }

        public void OnProtoMessage(Proto.QuoterReq req)
        {
            lock (this.quoters)
            {
                if (req.Type == Proto.RequestType.Set)
                {
                    foreach (var q in req.Quoters)
                    {
                        quoters[q.Name] = q;
                    }
                }
                else if (req.Type == Proto.RequestType.Del)
                {
                    foreach (var q in req.Quoters)
                    {
                        quoters.Remove(q.Name);
                    }
                }
            }
        }

        public Proto.QuoterSpec GetQuoter(string underlying)
        {
            lock (this.quoters)
            {
                foreach (var kvp in quoters)
                {
                    if (kvp.Value.Underlying == underlying)
                        return kvp.Value;
                }
            }
            return null;
        }

        public List<Proto.QuoterSpec> GetQuoters()
        {
            lock (this.quoters)
            {
                return this.quoters.Values.ToList();
            }
        }

        Dictionary<string, Proto.QuoterSpec> quoters = new Dictionary<string, Proto.QuoterSpec>();
    }
}
