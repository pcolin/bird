using Microsoft.Practices.Unity;
using Prism.Events;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.ViewModels
{
    class MonitorWindowViewModel
    {
        public MonitorWindowViewModel(IUnityContainer container)
        {
            this.Container = container;
            this.quoterAction = q =>
                {
                    MonitorUserControlViewModel vm = null;
                    if (this.viewModels.TryGetValue(q.Exchange, out vm))
                    {
                        if (q.Type == Proto.RequestType.Set)
                        {
                            foreach (var quoter in q.Quoters)
                            {
                                vm.RefreshQuoter(quoter);
                            }
                        }
                    }
                };

            this.switchAction = s =>
                {
                    MonitorUserControlViewModel vm = null;
                    if (this.viewModels.TryGetValue(s.Exchange, out vm))
                    {
                        foreach (var sw in s.Switches)
                        {
                            vm.RefreshStrategySwitch(sw);
                        }
                    }
                };

            this.statisticAction = s =>
                {
                    MonitorUserControlViewModel vm = null;
                    if (this.viewModels.TryGetValue(s.Exchange, out vm))
                    {
                        vm.RefreshStrategyStatistic(s);
                    }
                };
        }

        public void Start(Dictionary<Proto.Exchange, MonitorUserControlViewModel> viewModels)
        {
            this.viewModels = viewModels;
            this.Container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.QuoterReq>>().Subscribe(this.quoterAction, ThreadOption.BackgroundThread);
            this.Container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.StrategySwitchReq>>().Subscribe(this.switchAction, ThreadOption.BackgroundThread);
            this.Container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.StrategyStatistic>>().Subscribe(this.statisticAction, ThreadOption.BackgroundThread);
        }

        public IUnityContainer Container { get; set; }

        Action<Proto.QuoterReq> quoterAction;
        Action<Proto.StrategySwitchReq> switchAction;
        Action<Proto.StrategyStatistic> statisticAction;
        Dictionary<Proto.Exchange, MonitorUserControlViewModel> viewModels;
    }
}
