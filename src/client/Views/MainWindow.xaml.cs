using client.ViewModels;
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
    /// Interaction logic for MainWindow.xaml
    /// </summary>
    public partial class MainWindow : Window
    {
        public MainWindow()
        {
            InitializeComponent();
            //this.DataContext = new MainWindowViewModel();
        }

        private void LoginMenuItem_Click(object sender, RoutedEventArgs e)
        {
            LoginWindow login = new LoginWindow();
            login.DataContext = this.DataContext;
            login.Show();
        }
    }

    public class StatusColorConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            ConnectStatus status = (ConnectStatus)value;
            switch (status)
            {
                case ConnectStatus.Connected:
                    return "Green";
                case ConnectStatus.Disconnected:
                    return "Red";
                case ConnectStatus.Disabled:
                    return "Gray";
                default:
                    return "Gray";
            }
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }
    }
}
