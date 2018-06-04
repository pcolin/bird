using client.Models;
using Microsoft.Practices.Unity;
using Prism.Events;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Threading;

namespace client.ViewModels
{
    class VolatilityWindowViewModel : BindableBase
    {
        public IUnityContainer Container { get; set; }

        public VolatilityWindowViewModel(IUnityContainer container)
        {
            this.Container = container;

            this.PriceAction = p =>
            {
                VolatilityUserControlViewModel vm = null;
                if (this.viewModels.TryGetValue(p.Exchange, out vm))
                {
                    vm.ReceivePrice(p);
                }
            };

            this.SSRateReqAction = req =>
            {
                VolatilityUserControlViewModel vm = null;
                if (this.viewModels.TryGetValue(req.Exchange, out vm))
                {
                    vm.ReceiveSSRateReq(req);
                }
            };
        }

        public void Start(Dictionary<Proto.Exchange, VolatilityUserControlViewModel> viewModels)
        {
            this.viewModels = viewModels;

            this.Container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.Price>>().Subscribe(this.PriceAction, ThreadOption.BackgroundThread);
            this.Container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.SSRateReq>>().Subscribe(this.SSRateReqAction, ThreadOption.BackgroundThread);
        }

        Action<Proto.Price> PriceAction { get; set; }
        Action<Proto.SSRateReq> SSRateReqAction { get; set; }
        Dictionary<Proto.Exchange, VolatilityUserControlViewModel> viewModels;
    }
}
