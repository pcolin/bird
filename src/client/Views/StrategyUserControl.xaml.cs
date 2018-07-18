using client.ViewModels;
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
    /// Interaction logic for StrategyUserControl.xaml
    /// </summary>
    public partial class StrategyUserControl : UserControl
    {
        public StrategyUserControl(Proto.Exchange exchange, IUnityContainer container)
        {
            InitializeComponent();

            this.StrategyTabControl.Items.Add(new TabItem()
                {
                    Header = "Pricer",
                    Content = new PricerUserControl(exchange, container),
                });
            this.StrategyTabControl.Items.Add(new TabItem()
                {
                    Header = "Quoter",
                    Content = new QuoterUserControl(exchange, container),
                });
            this.StrategyTabControl.Items.Add(new TabItem()
                {
                    Header = "Hitter",
                    Content = new HitterUserControl(exchange, container),
                });
            this.StrategyTabControl.Items.Add(new TabItem()
                {
                    Header = "Dimer",
                    Content = new DimerUserControl(exchange, container),
                });
            this.StrategyTabControl.Items.Add(new TabItem()
                {
                    Header = "DummyQuoter",
                    Content = "To be continue...",
                });
        }
    }
}
