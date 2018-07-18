using Microsoft.Practices.Unity;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows;
using System.Windows.Controls;
using System.Windows.Data;
using System.Windows.Documents;
using System.Windows.Input;
using System.Windows.Media;
using System.Windows.Media.Imaging;
using System.Windows.Navigation;
using System.Windows.Shapes;

namespace client.Views
{
    /// <summary>
    /// Interaction logic for CreditUserControl.xaml
    /// </summary>
    public partial class CreditUserControl : UserControl
    {
        public CreditUserControl(Proto.Exchange exchange, IUnityContainer container)
        {
            InitializeComponent();

            this.StrategyCreditTabControl.Items.Add(new TabItem()
                {
                    Header = Proto.StrategyType.Quoter,
                    Content = new StrategyCreditUserControl(exchange, Proto.StrategyType.Quoter, container),
                });
            this.StrategyCreditTabControl.Items.Add(new TabItem()
                {
                    Header = Proto.StrategyType.Hitter,
                    Content = new StrategyCreditUserControl(exchange, Proto.StrategyType.Hitter, container),
                });
            this.StrategyCreditTabControl.Items.Add(new TabItem()
                {
                    Header = Proto.StrategyType.Dimer,
                    Content = new StrategyCreditUserControl(exchange, Proto.StrategyType.Dimer, container),
                });
        }
    }
}
