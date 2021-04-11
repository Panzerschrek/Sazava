#include "CentralWidget.hpp"
#include "Serialization.hpp"
#include <QtCore/QFile>
#include <QtWidgets/QApplication>
#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMainWindow>
#include <QtWidgets/QMenuBar>

namespace SZV
{

namespace
{

class MainWindow final : public QMainWindow
{
public:
	explicit MainWindow()
	{
		central_widget_= CreateCentralWidget(this);

		const auto menu_bar = new QMenuBar(this);
		const auto file_menu = menu_bar->addMenu("&File");
		file_menu->addAction("&Open", this, &MainWindow::OnOpen);
		file_menu->addAction("&Save", this, &MainWindow::OnSave);
		file_menu->addAction("&Quit", this, &QWidget::close);
		setMenuBar(menu_bar);

		setCentralWidget(central_widget_);

		setMinimumSize(640, 480);
		resize(1024, 768);
	}

private:
	void OnOpen()
	{
		const QString open_path= QFileDialog::getOpenFileName(this, "Sazava - open");
		if(open_path.isEmpty())
			return;

		QFile f(open_path);
		if(!f.open(QIODevice::ReadOnly))
			return;

		const QByteArray file_data= f.readAll();
		f.close();

		central_widget_->GetCSGTreeRoot()= DeserializeCSGExpressionTree(file_data);
	}

	void OnSave()
	{
		const QString save_path= QFileDialog::getSaveFileName(this, "Sazava - save");
		if(save_path.isEmpty())
			return;

		const QByteArray csg_tree_serialized= SerializeCSGExpressionTree(central_widget_->GetCSGTreeRoot());

		QFile f(save_path);
		if(!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
			return;

		f.write(csg_tree_serialized);
		f.close();
	}

private:
	CentralWidgetBase* central_widget_= nullptr;
};

int Main(int argc, char* argv[])
{
	QApplication app(argc, argv);

	MainWindow window;
	window.show();

	return app.exec();
}

} // namespace

} // namespace SZV

int main(int argc, char* argv[])
{
	return SZV::Main(argc, argv);
}
