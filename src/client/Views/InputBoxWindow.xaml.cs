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

namespace client.Views
{
    /// <summary>
    /// Interaction logic for InputBoxWindow.xaml
    /// </summary>
    public partial class InputBoxWindow : Window
    {
        public InputBoxWindow(string name)
        {
            InitializeComponent();

            this.Title = "Setting destriker";
            this.TextBlockName.Text = "New " + name + ":";
        }

        public double Value { get; set; }

        private void ButtonOK_Click(object sender, RoutedEventArgs e)
        {
            double value = 0;
            if (double.TryParse(this.TextBoxInput.Text, out value))
            {
                Value = value;
                this.DialogResult = true;
            }
            else
            {
                MessageBox.Show("Invalid input!");
                this.TextBoxInput.SelectAll();
            }
        }

        private void ButtonCancel_Click(object sender, RoutedEventArgs e)
        {
            this.Close();
        }
    }
}
