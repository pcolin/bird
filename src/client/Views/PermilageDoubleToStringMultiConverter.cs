﻿using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;

namespace client.Views
{
    class PermilageDoubleToStringMultiConverter : System.Windows.Data.IMultiValueConverter
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
                            return (value * 1000).ToString(format);
                        }
                    }
                    return (value * 1000).ToString(DefaultFormat);
                }
            }
            return null;
        }

        public object[] ConvertBack(object value, Type[] targetTypes, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
            //double result = 0;
            //double.TryParse(value.ToString(), out result);
            //return new object[3] { result / 1000, null, null };
        }
    }
}
