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
    /// Interaction logic for VolatilityWindow.xaml
    /// </summary>
    public partial class VolatilityWindow : Window
    {
        public VolatilityWindow(IUnityContainer container)
        {
            InitializeComponent();

            this.DataContext = new VolatilityWindowViewModel(container);
            container.Resolve<EventAggregator>().GetEvent<PubSubEvent<List<Proto.Exchange>>>().Subscribe(this.StartWindow, ThreadOption.PublisherThread);
        }

        private void StartWindow(List<Proto.Exchange> exchanges)
        {
            this.Dispatcher.BeginInvoke((MethodInvoker)delegate
            {
                var vm = this.DataContext as VolatilityWindowViewModel;
                var viewModels = new Dictionary<Proto.Exchange, VolatilityUserControlViewModel>();
                foreach (var exch in exchanges)
                {
                    var viewModel = new VolatilityUserControlViewModel(vm.Container, this.Dispatcher, exch);
                    var control = new VolatilityUserControl(viewModel);
                    viewModels[exch] = viewModel;
                    TabItem item = new TabItem() { Header = exch, Content = control };
                    this.VolatilityTabControl.Items.Add(item);
                }
                this.VolatilityTabControl.SelectedIndex = 0;
                vm.Start(viewModels);
            });
        }

        public void SaveAndClose(XmlWriter writer)
        {
            this.Dispatcher.Invoke(() =>
                {
                    MainWindow.WriteWindowPlacement(writer, this, "VolatilityWindow");

                    foreach (TabItem item in this.VolatilityTabControl.Items)
                    {
                        VolatilityUserControl control = item.Content as VolatilityUserControl;
                        if (control != null)
                        {
                            control.SaveLayout();
                        }
                    }

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
    }
}
