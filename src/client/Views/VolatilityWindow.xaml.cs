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
using System.Windows.Shapes;
using System.Xml;

namespace client.Views
{
    /// <summary>
    /// Interaction logic for VolatilityWindow.xaml
    /// </summary>
    public partial class VolatilityWindow : Window
    {
        public VolatilityWindow()
        {
            InitializeComponent();
        }

        public void SaveAndClose(XmlWriter writer)
        {
            this.Dispatcher.Invoke(() =>
                {
                    MainWindow.WriteWindowPlacement(writer, this, "VolatilityWindow");

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
