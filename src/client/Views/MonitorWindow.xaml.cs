using client.ViewModels;
using Microsoft.Practices.Unity;
using Prism.Events;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Forms;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Shapes;
using System.Xml;

namespace client.Views
{
    /// <summary>
    /// Interaction logic for MonitorWindow.xaml
    /// </summary>
    public partial class MonitorWindow : Window
    {
        public MonitorWindow(IUnityContainer container)
        {
            InitializeComponent();
            this.DataContext = new MonitorWindowViewModel(container);
            container.Resolve<EventAggregator>().GetEvent<PubSubEvent<List<Proto.Exchange>>>().Subscribe(this.StartWindow, ThreadOption.PublisherThread);
        }

        private void StartWindow(List<Proto.Exchange> exchanges)
        {
            this.Dispatcher.BeginInvoke((MethodInvoker)delegate
            {
                var vm = this.DataContext as MonitorWindowViewModel;
                var viewModels = new Dictionary<Proto.Exchange, MonitorUserControlViewModel>();
                foreach (var exch in exchanges)
                {
                    var viewModel = new MonitorUserControlViewModel(exch, vm.Container, this.Dispatcher);
                    viewModels[exch] = viewModel;
                    TabItem item = new TabItem() { Header = exch, Content = new MonitorUserControl(viewModel) };
                    this.MonitorTabControl.Items.Add(item);
                }
                if (exchanges.Count > 0)
                {
                    this.MonitorTabControl.SelectedIndex = 0;
                }
                vm.Start(viewModels);
            });
        }

        public void SaveAndClose(XmlWriter writer)
        {
            this.Dispatcher.Invoke(() =>
            {
                MainWindow.WriteWindowPlacement(writer, this, "MonitorWindow");

                this.close = true;
                this.Close();
            });
        }

        private void Window_Closing(object sender, System.ComponentModel.CancelEventArgs e)
        {
            if (close == false)
            {
                this.Hide();
                e.Cancel = true;
                return;
            }
        }

        private bool close = false;
        //private IUnityContainer container;
    }
}
