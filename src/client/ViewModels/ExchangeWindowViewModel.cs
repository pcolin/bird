using Microsoft.Practices.Unity;
using Prism.Events;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using client.Models;
using System.Globalization;

namespace client.ViewModels
{
    class ExchangeWindowViewModel : BindableBase
    {
        private readonly ObservableCollection<BindableBase> tabItems = new ObservableCollection<BindableBase>();
        public ObservableCollection<BindableBase> TabItems
        {
            get { return tabItems; }
        }

        private BindableBase selectedTab;
        public BindableBase SelectedTab
        {
            get { return selectedTab; }
            set { SetProperty(ref selectedTab, value); }
        }

        public ExchangeWindowViewModel(IUnityContainer container, List<Proto.Exchange> exchanges)
        {
            this.container = container;
            this.container.Resolve<EventAggregator>().GetEvent<StartWindowEvent>().Subscribe(this.StartWindow, ThreadOption.PublisherThread);
            this.exchanges = exchanges;
        }

        private void StartWindow()
        {
            foreach (var exchange in exchanges)
            {
                ExchangeUserControlViewModel vm = new ExchangeUserControlViewModel(this.container, exchange);
                vm.Refresh();
                tabItems.Add(vm);
            }
        }

        private IUnityContainer container;
        List<Proto.Exchange> exchanges;
    }
}
