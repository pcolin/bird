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
    /// Interaction logic for CreditWindow.xaml
    /// </summary>
    public partial class CreditWindow : Window
    {
        public CreditWindow(IUnityContainer container)
        {
            InitializeComponent();
            this.container = container;
            container.Resolve<EventAggregator>().GetEvent<PubSubEvent<List<Proto.Exchange>>>().Subscribe(this.StartWindow, ThreadOption.PublisherThread);
        }

        private void StartWindow(List<Proto.Exchange> exchanges)
        {
            this.Dispatcher.BeginInvoke((MethodInvoker)delegate
            {
                foreach (var exch in exchanges)
                {
                    var control = new CreditUserControl(exch, this.container);
                    TabItem item = new TabItem() { Header = exch, Content = control };
                    this.CreditTabControl.Items.Add(item);
                }
                if (exchanges.Count > 0)
                {
                    this.CreditTabControl.SelectedIndex = 0;
                }
            });
        }

        public void SaveAndClose(XmlWriter writer)
        {
            this.Dispatcher.Invoke(() =>
            {
                MainWindow.WriteWindowPlacement(writer, this, "CreditWindow");

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
        private IUnityContainer container;
    }
}
