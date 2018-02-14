using client.Models;
using client.Views;
using Microsoft.Practices.Unity;
using Prism.Unity;
using System.Windows;

namespace client
{
    class Bootstrapper : UnityBootstrapper
    {
        protected override DependencyObject CreateShell()
        {
            return Container.Resolve<MainWindow>();
        }

        protected override void InitializeShell()
        {
            Application.Current.MainWindow.Show();
        }

        protected override void ConfigureContainer()
        {
            base.ConfigureContainer();

            //this.Container.RegisterInstance<ServerConnector>(new ServerConnector());
            //this.Container.RegisterType<IConnector, ServerConnector>(new ContainerControlledLifetimeManager());
            //this.Container.RegisterType<ISocket, ServerService>(new ContainerControlledLifetimeManager());
            //this.Container.RegisterType<ISocket, ProxyService>(new ContainerControlledLifetimeManager());
            //this.Container.RegisterInstance<ServerService>(new ServerService());
        }
    }
}
