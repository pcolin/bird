﻿using client.ViewModels;
using Microsoft.Practices.Unity;
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
    /// Interaction logic for StrategyCreditUserControl.xaml
    /// </summary>
    public partial class StrategyCreditUserControl : UserControl
    {
        public StrategyCreditUserControl(Proto.Exchange exchange, Proto.StrategyType strategy, IUnityContainer container)
        {
            InitializeComponent();

            this.DataContext = new StrategyCreditUserControlViewModel(exchange, strategy, container);
        }
    }
}
