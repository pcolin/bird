using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Data;

namespace client.Views
{
    class DoubleToStringMultiConverter : IMultiValueConverter
    {
        public string DefaultFormat { get; set; }

        public object Convert(object[] values, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            if (values[0] is double)
            {
                double value = (double)values[0];
                if (double.IsNaN(value) == false)
                {
                    Dictionary<int, string> formats = values[2] as Dictionary<int, string>;
                    if (formats != null)
                    {
                        int index = (int)values[1];
                        string format = null;
                        if (formats.TryGetValue(index, out format))
                        {
                            return value.ToString(format);
                        }
                    }
                    return value.ToString(DefaultFormat);
                }
                //return string.Empty;
            }
            return null;
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
        {
            double result = 0;
            double.TryParse(value.ToString(), out result);
            return new object[3] { result, null, null };
        }
    }
}
