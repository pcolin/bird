using System;
using System.Collections.Generic;
using System.Globalization;
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
    /// Interaction logic for ExchangeUserControl.xaml
    /// </summary>
    public partial class ExchangeUserControl : UserControl
    {
        public ExchangeUserControl()
        {
            InitializeComponent();
        }
    }

    public class DateConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            DateTime dt = (DateTime)value;
            return dt.ToString("yyyyMMdd");
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            try
            {
                return DateTime.ParseExact(value.ToString(), "yyyyMMdd", CultureInfo.InvariantCulture);
            }
            catch (Exception)
            {
                return new ValidationResult(false, "Invalid input");
            }
        }
    }

    public class TimeConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            DateTime dt = (DateTime)value;
            return dt.ToString("HH:mm:ss");
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            try
            {
                return DateTime.ParseExact(value.ToString(), "HH:mm:ss", CultureInfo.InvariantCulture);
            }
            catch (Exception)
            {
                return new ValidationResult(false, "Invalid input");
            }
        }
    }

    public class WeightConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            double d = (double)value;
            return d.ToString();
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            double d = 0;
            if (double.TryParse(value.ToString(), out d) && d >= 0 && d <= 1)
            {
                return d;
            }
            else
            {
                return new ValidationResult(false, "Invalid input");
            }
        }
    }
}
