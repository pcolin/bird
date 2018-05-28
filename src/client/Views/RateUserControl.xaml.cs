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
    /// Interaction logic for RateUserControl.xaml
    /// </summary>
    public partial class RateUserControl : UserControl
    {
        public RateUserControl()
        {
            InitializeComponent();
        }
    }

    public class DaysConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            int i = (int)value;
            return i.ToString();
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            int i = 0;
            if (int.TryParse(value.ToString(), out i) && i > 0)
            {
                return i;
            }
            else
            {
                return new ValidationResult(false, "Invalid input");
            }
        }
    }

    public class RateConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            double d = (double)value;
            return (d * 100).ToString();
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            double d = 0;
            if (double.TryParse(value.ToString(), out d))
            {
                return d / 100;
            }
            else
            {
                return new ValidationResult(false, "Invalid input");
            }
        }
    }
}
