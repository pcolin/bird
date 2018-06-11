using Microsoft.Practices.Unity;
using Prism.Events;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Collections.ObjectModel;
using System.ComponentModel;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using System.Windows.Threading;

namespace client.ViewModels
{
    class MessageWindowViewModel : BindableBase
    {
        private ObservableCollection<Proto.ServerInfo> messages;
        public ObservableCollection<Proto.ServerInfo> Messages
        {
            get { return messages; }
            set { SetProperty(ref messages, value); }
        }

        Action<Proto.ServerInfo> InfoAction { get; set; }
        IUnityContainer Container { get; set; }

        public MessageWindowViewModel(IUnityContainer container, Dispatcher dispatcher)
        {
            InfoAction = new Action<Proto.ServerInfo>(i => dispatcher.BeginInvoke((MethodInvoker)delegate
            {
                this.Messages.Add(i);
            }));
            messages = new ObservableCollection<Proto.ServerInfo>();

            container.Resolve<EventAggregator>().GetEvent<PubSubEvent<List<Proto.Exchange>>>().Subscribe(this.StartWindow, ThreadOption.PublisherThread);
            this.Container = container;
        }

        private void StartWindow(List<Proto.Exchange> exchanges)
        {
            if (exchanges.Count > 0)
            {
                Container.Resolve<EventAggregator>().GetEvent<PubSubEvent<Proto.ServerInfo>>().Subscribe(this.InfoAction, ThreadOption.PublisherThread);
            }
        }
    }
}
