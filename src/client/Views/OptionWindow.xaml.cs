using client.ViewModels;
using Microsoft.Practices.Unity;
using Prism.Events;
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
    /// Interaction logic for OptionWindow.xaml
    /// </summary>
    public partial class OptionWindow : Window
    {
        public OptionWindow(IUnityContainer container, Proto.Exchange exchange)
        {
            InitializeComponent();

            this.DataContext = new OptionWindowViewModel(container, this.Dispatcher, exchange);
            
            //container.Resolve<EventAggregator>().GetEvent<CloseWindowEvent>().Subscribe(this.CloseWindow, ThreadOption.PublisherThread);
        }

        public void SaveAndClose(XmlWriter writer)
        {
            this.Dispatcher.Invoke(() =>
            {
                SaveLayout(writer);
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

        void SaveLayout(XmlWriter writer)
        {
            MainWindow.WriteWindowPlacement(writer, this, "OptionWindow_" + (this.DataContext as OptionWindowViewModel).Exchange);

            var control = GetUserControl(this.OptionTabControl);
            if (control != null)
            {
                control.SaveLayout();
            }
        }

        OptionUserControl GetUserControl(DependencyObject obj)
        {
            int count = VisualTreeHelper.GetChildrenCount(obj);
            for (int i = 0; i < count; ++i)
            {
                var child = VisualTreeHelper.GetChild(obj, i);
                if (child != null)
                {
                    if (child is OptionUserControl)
                    {
                        return (OptionUserControl)child;
                    }
                    else
                    {
                        return GetUserControl(child);
                    }
                }
            }
            return null;
        }

        private bool close = false;
    }
}
