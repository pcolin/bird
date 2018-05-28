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
        private readonly ObservableCollection<VolatilityUserControlViewModel> tabItems = new ObservableCollection<VolatilityUserControlViewModel>();
        public ObservableCollection<VolatilityUserControlViewModel> TabItems
        {
            get { return tabItems; }
        }

        private BindableBase selectedTab;
        public BindableBase SelectedTab
        {
            get { return selectedTab; }
            set { SetProperty(ref selectedTab, value); }
        }

        public VolatilityWindowViewModel(IUnityContainer container, Dispatcher dispatcher, List<Proto.Exchange> exchanges)
        {
            this.container = container;
            this.dispatcher = dispatcher;
            this.exchanges = exchanges;

            this.container.Resolve<EventAggregator>().GetEvent<StartWindowEvent>().Subscribe(this.StartWindow, ThreadOption.PublisherThread);

        }

        private void StartWindow()
        {
            this.dispatcher.BeginInvoke((MethodInvoker)delegate
            {
                foreach (var exch in this.exchanges)
                {
                    VolatilityUserControlViewModel vm = new VolatilityUserControlViewModel(this.container, this.dispatcher, exch);
                    vm.Initialize();
                    tabItems.Add(vm);
                    //this.viewModels[exch] = vm;
                }
            });
        }

        private IUnityContainer container;
        private Dispatcher dispatcher;
        List<Proto.Exchange> exchanges;
        //Dictionary<Proto.Exchange, VolatilityUserControlViewModel> viewModels = new Dictionary<Proto.Exchange, VolatilityUserControlViewModel>();
    }
}
