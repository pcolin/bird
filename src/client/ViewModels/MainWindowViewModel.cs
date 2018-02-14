using client.Models;
using Microsoft.Practices.Unity;
using Prism.Commands;
using Prism.Interactivity.InteractionRequest;
using Prism.Mvvm;
using System;
using System.Collections.Generic;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Input;

namespace client.ViewModels
{
    public enum Exchange
    {
        DCE = 0,
        CZCE,
        CFFEX,
        SHFE,
    };

    public enum ConnectStatus
    {
        Connected = 0,
        Disconnected = 1,
        Disabled = 2,
    };

    class MainWindowViewModel : BindableBase
    {
        public ICommand InitializeCommand { get; set; }
        public ICommand LoginCommand { get; set; }
        public ICommand LogoutCommand { get; set; }

        public InteractionRequest<INotification> NotificationRequest { get; set; }

        private string version = "0.2.01";
        public string Version
        {
            get { return version; }
            //set { SetProperty(ref version, value); }
        }

        public string Title
        {
            get { return "Bird " + Version; }
        }

        private Exchange exchange1 = Exchange.DCE;
        public Exchange Exchange1
        {
            get { return exchange1; }
            set { SetProperty(ref this.exchange1, value); }
        }

        private bool isExchange1Selected;
        public bool IsExchange1Selected
        {
            get { return isExchange1Selected; }
            set
            {
                if (SetProperty(ref isExchange1Selected, value))
                {
                    (LoginCommand as DelegateCommand).RaiseCanExecuteChanged();
                }
            }
        }        

        private ConnectStatus exchange1Status = ConnectStatus.Disabled;
        public ConnectStatus Exchange1Status
        {
            get { return exchange1Status; }
            set
            {
                if (SetProperty(ref exchange1Status, value))
                {
                    (LogoutCommand as DelegateCommand).RaiseCanExecuteChanged();
                }
            }
        }        

        private Exchange exchange2 = Exchange.CZCE;
        public Exchange Exchange2
        {
            get { return exchange2; }
            set { SetProperty(ref exchange2, value); }
        }

        private bool isExchange2Selected;
        public bool IsExchange2Selected
        {
            get { return isExchange2Selected; }
            set
            {
                if (SetProperty(ref isExchange2Selected, value))
                {
                    (LoginCommand as DelegateCommand).RaiseCanExecuteChanged();
                }
            }
        }

        private ConnectStatus exchange2Status = ConnectStatus.Disabled;
        public ConnectStatus Exchange2Status
        {
            get { return exchange2Status; }
            set
            {
                if (SetProperty(ref exchange2Status, value))
                {
                    (LogoutCommand as DelegateCommand).RaiseCanExecuteChanged();
                }
            }
        }

        private Exchange exchange3 = Exchange.CFFEX;
        public Exchange Exchange3
        {
            get { return exchange3; }
            set { SetProperty(ref exchange3, value); }
        }

        private bool isExchange3Selected;
        public bool IsExchange3Selected
        {
            get { return isExchange3Selected; }
            set
            {
                if (SetProperty(ref isExchange3Selected, value))
                {
                    (LoginCommand as DelegateCommand).RaiseCanExecuteChanged();
                }
            }
        }

        private ConnectStatus exchange3Status = ConnectStatus.Disabled;
        public ConnectStatus Exchange3Status
        {
            get { return exchange3Status; }
            set
            {
                if (SetProperty(ref exchange3Status, value))
                {
                    (LogoutCommand as DelegateCommand).RaiseCanExecuteChanged();
                }
            }
        }

        private Exchange exchange4 = Exchange.SHFE;
        public Exchange Exchange4
        {
            get { return exchange4; }
            set { SetProperty(ref this.exchange4, value); }
        }

        private bool isExchange4Selected;
        public bool IsExchange4Selected
        {
            get { return isExchange4Selected; }
            set
            {
                if (SetProperty(ref isExchange4Selected, value))
                {
                    (LoginCommand as DelegateCommand).RaiseCanExecuteChanged();
                }
            }
        }

        private ConnectStatus exchange4Status = ConnectStatus.Disabled;
        public ConnectStatus Exchange4Status
        {
            get { return exchange4Status; }
            set
            {
                if (SetProperty(ref exchange4Status, value))
                {
                    (LogoutCommand as DelegateCommand).RaiseCanExecuteChanged();
                }
            }
        }

        private string user = "colin";
        public string User
        {
            get { return user; }
            set { SetProperty(ref user, value); }
        }

        private string password = "";
        public string Password
        {
            get { return password; }
            set { SetProperty(ref password, value); }
        }        
        
        public MainWindowViewModel(IUnityContainer container)
        {
            //LoginCommand = new DelegateCommand<object>(new Action<object>(this.LoginExecute), new Func<object, bool>(this.CanLogin));
            InitializeCommand = new DelegateCommand(this.InitializeExecute);
            LoginCommand = new DelegateCommand(this.LoginExecute, this.CanLogin);
            LogoutCommand = new DelegateCommand(this.LogoutExecute, this.CanLogout);

            NotificationRequest = new InteractionRequest<INotification>();
            //this.serverService = service;
            this.container = container;
        }

        private void InitializeExecute()
        {
            /// Load config from xml
            //addresses = new Dictionary<Exchange, Tuple<string, string, string>>();
            servers = new Dictionary<Exchange, ServerService>();
            string server = "tcp://172.28.1.53:8001";
            string database = "tcp://172.28.1.53:8002";
            string proxy = "tcp://172.28.1.53:8003";
            var service = new ServerService();
            this.container.RegisterInstance<ServerService>(Exchange.DCE.ToString(), service);
            service.Initialize(server);
            servers.Add(Exchange1, service);


            //proxyService = new ProxyService();
            //this.container.RegisterInstance<ProxyService>(Exchange.DCE.ToString(), proxyService);
            //proxyService.Initialize(proxy);
        }
       
        private void LoginExecute()
        {
            if (IsExchange1Selected)
            {
                ServerService s = null;
                if (this.servers.TryGetValue(Exchange1, out s))
                {
                    var r = s.Login(this.User, this.Password, this.Version);
                    if (r != null)
                    {
                        if (r.Result)
                        {
                            Exchange1Status = ConnectStatus.Connected;
                            s.Start();
                        }
                        else
                        {
                            NotificationRequest.Raise(new Notification { Content = r.Error, Title = "Login Failed" });
                        }
                    }
                    else
                    {
                        NotificationRequest.Raise(new Notification { Content = "Login timeout", Title = "Login Failed" });
                    }
                }
            }

            if (IsExchange2Selected)
            {
                Exchange2Status = ConnectStatus.Connected;
            }

            if (IsExchange3Selected)
            {
                Exchange3Status = ConnectStatus.Connected;
            }

            if (IsExchange4Selected)
            {
                Exchange4Status = ConnectStatus.Connected;
            }
        }

        private bool CanLogin()
        {
            return IsExchange1Selected || IsExchange2Selected || IsExchange3Selected || IsExchange4Selected;
        }

        private void LogoutExecute()
        {
            foreach (var kvp in servers)
            {
                kvp.Value.Logout(this.User);
            }
        }

        private bool CanLogout()
        {
            return Exchange1Status == ConnectStatus.Connected || Exchange2Status == ConnectStatus.Connected ||
                   Exchange3Status == ConnectStatus.Connected || exchange4Status == ConnectStatus.Connected;
        }

        IUnityContainer container;
        private Dictionary<Exchange, ServerService> servers;
        private Dictionary<Exchange, ProxyService> proxies;
    }
}
