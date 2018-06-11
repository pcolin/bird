﻿using Proto;
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
    /// Interaction logic for MessageWindow.xaml
    /// </summary>
    public partial class MessageWindow : Window
    {
        public MessageWindow()
        {
            InitializeComponent();
        }

        public void SaveAndClose(XmlWriter writer)
        {
            this.Dispatcher.Invoke(() =>
                {
                    MainWindow.WriteWindowPlacement(writer, this, "MessageWindow");

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

        private void ListViewItem_MouseDoubleClick(object sender, MouseButtonEventArgs e)
        {
            var info = ((ListViewItem)sender).Content as Proto.ServerInfo;
            if (info != null)
            {
                MessageBox.Show(info.Info, info.Type.ToString());
            }
        }

        private bool close = false;
    }

    public class MessageTimeConverter : IValueConverter
    {
        public object Convert(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            ulong t = (ulong)value;
            var time = this.dt.AddSeconds(t);
            return time.ToString("HH:mm:ss");
        }

        public object ConvertBack(object value, Type targetType, object parameter, System.Globalization.CultureInfo culture)
        {
            throw new NotImplementedException();
        }

        private DateTime dt = new DateTime(1970, 1, 1, 0, 0, 0).ToLocalTime();
    }

    public class DefaultErrorStyleSelector : StyleSelector
    {
        public Style DefaultStyle { get; set; }
        public Style ErrorStyle { get; set; }

        public override Style SelectStyle(object item, DependencyObject container)
        {
            ServerInfo info = (ServerInfo)item;

            if (info.Type == ServerInfo.Types.Type.Info)
            {
                return DefaultStyle;
            }
            else
            {
                return ErrorStyle;
            }
        }
    }
}
