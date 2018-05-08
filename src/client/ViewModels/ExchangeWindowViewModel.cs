using Microsoft.Practices.Unity;
using Prism.Events;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

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

        public ExchangeWindowViewModel(IUnityContainer container)
        {
            this.container = container;
            this.container.Resolve<EventAggregator>().GetEvent<StartWindowEvent>().Subscribe(this.StartWindow, ThreadOption.PublisherThread);
        }

        private void StartWindow()
        {
            ExchangeUserControlViewModel vm = new ExchangeUserControlViewModel();
            vm.Exchange = Proto.Exchange.De;
            tabItems.Add(vm);
        }

        private IUnityContainer container;
    }
}
