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
    /// Interaction logic for InputBoxUserControl.xaml
    /// </summary>
    public partial class InputBoxUserControl : UserControl
    {
        public InputBoxUserControl()
        {
            InitializeComponent();
        }

        public void Show(string name)
        {
            this.TextBoxValue.Text = name;
            this.TextBoxValue.Clear();
            Value = null;
            this.Visibility = Visibility.Visible;
        }

        public double? Value { get; set; }

        private void ButtonOK_Click(object sender, RoutedEventArgs e)
        {
            double value = 0;
            if (double.TryParse(this.TextBoxValue.Text, out value))
            {
                Value = value;
                this.Visibility = Visibility.Collapsed;
            }
            else
            {
                MessageBox.Show("Invalid input!");
                this.TextBoxValue.SelectAll();
            }
        }

        private void ButtonCancel_Click(object sender, RoutedEventArgs e)
        {
            Value = null;
            this.Visibility = Visibility.Collapsed;
        }
    }
}
