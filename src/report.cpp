/***********************************************************************
 *
 * Copyright (C) 2009 Graeme Gott <graeme@gottcode.org>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 ***********************************************************************/

#include "report.h"

#include "data_model.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFormLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLineEdit>
#include <QPrintDialog>
#include <QPrinter>
#include <QPushButton>
#include <QSettings>
#include <QTabWidget>
#include <QTextEdit>
#include <QTreeView>
#include <QVBoxLayout>

/*****************************************************************************/

Report::Report(DataModel* data, QWidget* parent)
: QDialog(parent), m_data(data) {
	setWindowTitle(tr("Time Sheet Report"));

	// Create contact information widgets
	QWidget* contact_info_tab = new QWidget(this);
	m_name = new QLineEdit(contact_info_tab);
	connect(m_name, SIGNAL(textChanged(const QString&)), this, SLOT(generateText()));
	m_company = new QLineEdit(contact_info_tab);
	connect(m_company, SIGNAL(textChanged(const QString&)), this, SLOT(generateText()));
	m_address = new QTextEdit(contact_info_tab);
	connect(m_address, SIGNAL(textChanged()), this, SLOT(generateText()));
	m_phone = new QLineEdit(contact_info_tab);
	connect(m_phone, SIGNAL(textChanged(const QString&)), this, SLOT(generateText()));
	m_fax = new QLineEdit(contact_info_tab);
	connect(m_fax, SIGNAL(textChanged(const QString&)), this, SLOT(generateText()));
	m_email = new QLineEdit(contact_info_tab);
	connect(m_email, SIGNAL(textChanged(const QString&)), this, SLOT(generateText()));
	m_website = new QLineEdit(contact_info_tab);
	connect(m_website, SIGNAL(textChanged(const QString&)), this, SLOT(generateText()));

	QFormLayout* contact_info_layout = new QFormLayout(contact_info_tab);
	contact_info_layout->addRow(tr("Name:"), m_name);
	contact_info_layout->addRow(tr("Company:"), m_company);
	contact_info_layout->addRow(tr("Address:"), m_address);
	contact_info_layout->addRow(tr("Phone:"), m_phone);
	contact_info_layout->addRow(tr("Fax:"), m_fax);
	contact_info_layout->addRow(tr("Email:"), m_email);
	contact_info_layout->addRow(tr("Website:"), m_website);

	// Create data widgets
	QWidget* data_tab = new QWidget(this);

	m_groups = new QComboBox(data_tab);
	connect(m_groups, SIGNAL(currentIndexChanged(int)), this, SLOT(groupSelected(int)));

	m_details = new QTreeView(data_tab);
	m_details->setEditTriggers(QAbstractItemView::NoEditTriggers);
	m_details->setRootIsDecorated(false);
	m_details->setSelectionMode(QAbstractItemView::SingleSelection);
	m_details->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_details->setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
	m_details->header()->setClickable(false);
	m_details->header()->setMovable(false);
	m_details->header()->setResizeMode(QHeaderView::ResizeToContents);

	m_details->setModel(m_data);
	m_details->header()->setResizeMode(3, QHeaderView::Stretch);
	m_details->header()->setStretchLastSection(false);
	for (int i = 5; i < 10; ++i) {
		m_details->setColumnHidden(i, true);
	}

	m_hourly_rate = new QDoubleSpinBox(data_tab);
	m_hourly_rate->setSpecialValueText(tr("N/A"));
	connect(m_hourly_rate, SIGNAL(valueChanged(double)), this, SLOT(generateText()));
	m_tax_rate = new QDoubleSpinBox(data_tab);
	m_tax_rate->setSuffix(QLocale().percent());
	m_tax_rate->setSpecialValueText(tr("N/A"));
	connect(m_tax_rate, SIGNAL(valueChanged(double)), this, SLOT(generateText()));
	m_currency_symbol = new QLineEdit(data_tab);
	connect(m_currency_symbol, SIGNAL(textChanged(const QString&)), this, SLOT(currencyChanged()));
	m_prepend_symbol = new QCheckBox(tr("Prepend currency symbol"), data_tab);
	connect(m_prepend_symbol, SIGNAL(toggled(bool)), this, SLOT(currencyChanged()));

	QFormLayout* data_rates_layout = new QFormLayout;
	data_rates_layout->setFormAlignment(Qt::AlignCenter | Qt::AlignTop);
	data_rates_layout->setFieldGrowthPolicy(QFormLayout::AllNonFixedFieldsGrow);
	data_rates_layout->setMargin(0);
	data_rates_layout->addRow(tr("Hourly rate:"), m_hourly_rate);
	data_rates_layout->addRow(tr("Tax rate:"), m_tax_rate);
	data_rates_layout->addRow(tr("Currency symbol:"), m_currency_symbol);
	data_rates_layout->addRow("", m_prepend_symbol);

	QVBoxLayout* data_layout = new QVBoxLayout(data_tab);
	data_layout->addWidget(m_groups);
	data_layout->addWidget(m_details);
	data_layout->addLayout(data_rates_layout);

	// Create preview widget
	m_preview = new QTextEdit(this);
	m_preview->setReadOnly(true);
	m_preview->document()->setDefaultStyleSheet("td { padding: 0 5px; } th { padding: 0 5px; }");

	// Create tabs
	QTabWidget* tabs = new QTabWidget(this);
	tabs->addTab(contact_info_tab, tr("Contact Information"));
	tabs->addTab(data_tab, tr("Data"));
	tabs->addTab(m_preview, tr("Preview"));

	// Create dialog actions
	QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Close | QDialogButtonBox::Reset, Qt::Horizontal, this);
	QPushButton* print_button = buttons->addButton(tr("Print"), QDialogButtonBox::ActionRole);
	connect(buttons, SIGNAL(rejected()), this, SLOT(reject()));
	connect(buttons->button(QDialogButtonBox::Reset), SIGNAL(clicked()), this, SLOT(reset()));
	connect(print_button, SIGNAL(clicked()), this, SLOT(print()));

	// Lay out dialog
	QVBoxLayout* layout = new QVBoxLayout(this);
	layout->addWidget(tabs, 1);
	layout->addWidget(buttons);

	resize(QSettings().value("ReportDialog/Size", QSize(700, 500)).toSize());

	// Load data
	findGroups();
	reset();
}

/*****************************************************************************/

void Report::hideEvent(QHideEvent* event) {
	QString address = m_address->toPlainText();
	address.replace("\n", "\\n");

	QSettings settings;
	settings.setValue("ReportDialog/Size", size());
	settings.setValue("ReportDialog/HourlyRate", m_hourly_rate->value());
	settings.setValue("ReportDialog/TaxRate", m_tax_rate->value());
	settings.setValue("ReportDialog/Currency", m_currency_symbol->text().simplified());
	settings.setValue("ReportDialog/PrependCurrency", m_prepend_symbol->isChecked());
	settings.setValue("Contact/Name", m_name->text());
	settings.setValue("Contact/Company", m_company->text());
	settings.setValue("Contact/Address", address);
	settings.setValue("Contact/Phone", m_phone->text());
	settings.setValue("Contact/Fax", m_fax->text());
	settings.setValue("Contact/Email", m_email->text());
	settings.setValue("Contact/Website", m_website->text());

	QDialog::hideEvent(event);
}

/*****************************************************************************/

void Report::currencyChanged() {
	QString symbol = m_currency_symbol->text().simplified();
	if (m_prepend_symbol->isChecked()) {
		m_hourly_rate->setSuffix("");
		m_hourly_rate->setPrefix(symbol);
	} else {
		m_hourly_rate->setPrefix("");
		m_hourly_rate->setSuffix(symbol);
	}
	generateText();
}

/*****************************************************************************/

void Report::generateText() {
	if (!m_data->rowCount()) {
		m_preview->clear();
		return;
	}

	QString html = "<p><center><b><big>" + tr("Time Sheet Report") + "</big><br />\n" + m_groups->currentText() + "</b>\n<small>";

	// Add contact information
	QString info = m_name->text().simplified();
	if (!info.isEmpty()) {
		html += "<br />\n" + info;
	}
	info = m_company->text().simplified();
	if (!info.isEmpty()) {
		html += "<br />\n" + info;
	}
	info = m_address->toPlainText().simplified();
	if (!info.isEmpty()) {
		info.replace("\n", "<br />\n");
		html += "<br />\n" + info;
	}
	info = m_phone->text().simplified();
	if (!info.isEmpty()) {
		html += "<br />\n" + tr("Phone: %1").arg(info);
	}
	info = m_fax->text().simplified();
	if (!info.isEmpty()) {
		html += "<br />\n" + tr("Fax: %1").arg(info);
	}
	info = m_email->text().simplified();
	if (!info.isEmpty()) {
		html += "<br />\n" + info;
	}
	info = m_website->text().simplified();
	if (!info.isEmpty()) {
		html += "<br />\n" + info;
	}

	// Finish header
	html += "<br />\n</small></center></p>\n";

	// Add data
	html += "<table width=\"100%\">\n";

	QString header("<tr>"
		"<th align=\"left\">%1</th>"
		"<th align=\"left\">%2</th>"
		"<th align=\"left\">%3</th>"
		"<th align=\"center\" colspan=\"2\">%4</th>"
		"<th align=\"right\">%5</th>"
		"</tr>\n");
	for (int column = 0; column < 5; ++column) {
		header = header.arg(m_data->headerData(column, Qt::Horizontal).toString());
	}
	html += header;

	int last = 0;
	int rows = m_data->rowCount();
	for (int row = 0; row < rows; ++row) {
		if (m_details->isRowHidden(row, QModelIndex())) {
			continue;
		}
		last = row;

		QStringList columns;
		for (int column = 0; column < 5; ++column) {
			QModelIndex index = m_data->index(row, column);
			QString data = m_data->data(index).toString().simplified();
			if (column != 3) {
				data.replace(" ", "&nbsp;");
			}
			columns.append(data);
		}
		html += QString("<tr>"
			"<td width=\"0%\" align=\"right\">%1</td>"
			"<td width=\"0%\" align=\"right\">%2</td>"
			"<td width=\"0%\" align=\"right\">%3</td>"
			"<td width=\"100%\" colspan=\"2\">%4</td>"
			"<td width=\"0%\" align=\"right\">%5</td>"
			"</tr>\n").arg(columns[0]).arg(columns[1]).arg(columns[2]).arg(columns[3]).arg(columns[4]);
	}

	// Add billing information
	QString hours = m_data->session(last).total(Session::Total, true);
	double value = hours.toDouble();
	html += "<tr><td colspan=\"4\"></td><td colspan=\"2\"><hr></td>\n";
	html += "<tr><td colspan=\"4\"></td><td>" + tr("Hours") + "</td><td align=\"right\">" + hours + "</td></tr>\n";
	double hourly_rate = m_hourly_rate->value();
	QString currency(m_hourly_rate->prefix() + "%1" + m_hourly_rate->suffix());
	if (hourly_rate) {
		value *= hourly_rate;
		double tax_rate = m_tax_rate->value() * 0.01;
		if (tax_rate) {
			html += "<tr><td colspan=\"4\"</td><td>" + tr("Subtotal") + "</td><td align=\"right\">" + QString(currency).arg(value, 0, 'f', 2) + "</td></tr>\n";
			double tax = tax_rate * value;
			html += "<tr><td colspan=\"4\"></td><td>" + tr("Taxes") + "</td><td align=\"right\">" + QString(currency).arg(tax, 0, 'f', 2) + "</td></tr>\n";
			value += tax;
		}
		html += "<tr><td colspan=\"4\"></td><td><b>" + tr("Total") + "</b></td><td align=\"right\"><b>" + QString(currency).arg(value, 0, 'f', 2) + "</b></td></tr>\n";
	}

	html += "</table>\n";

	m_preview->setHtml(html);
}

/*****************************************************************************/

void Report::groupSelected(int group) {
	if (group == -1) {
		return;
	}

	for (int i = 0; i < m_data->rowCount(); ++i) {
		m_details->setRowHidden(i, QModelIndex(), true);
	}

	QList<QVariant> rows = m_groups->itemData(group).toList();
	foreach (const QVariant& row, rows) {
		m_details->setRowHidden(row.toInt(), QModelIndex(), false);
	}

	generateText();
}

/*****************************************************************************/

void Report::print() {
	QPrinter printer;
	QPrintDialog dialog(&printer, this);
	if (dialog.exec() != QDialog::Accepted) {
		return;
	}
	m_preview->print(&printer);
}

/*****************************************************************************/

void Report::reset() {
	QSettings settings;

	QString address = settings.value("Contact/Address").toString();
	address.replace("\\n", "\n");

	m_hourly_rate->setValue(settings.value("ReportDialog/HourlyRate").toDouble());
	m_tax_rate->setValue(settings.value("ReportDialog/TaxRate").toDouble());
	m_currency_symbol->setText(settings.value("ReportDialog/Currency", "$").toString());
	m_prepend_symbol->setChecked(settings.value("ReportDialog/PrependCurrency", true).toBool());
	m_name->setText(settings.value("Contact/Name").toString());
	m_company->setText(settings.value("Contact/Company").toString());
	m_address->setPlainText(address);
	m_phone->setText(settings.value("Contact/Phone").toString());
	m_fax->setText(settings.value("Contact/Fax").toString());
	m_email->setText(settings.value("Contact/Email").toString());
	m_website->setText(settings.value("Contact/Website").toString());
}

/*****************************************************************************/

void Report::findGroups() {
	int count = m_data->rowCount();
	QList<int> billed = m_data->billedRows();
	if (!billed.contains(count - 2)) {
		billed.append(count - 2);
	}
	QList<QVariant> rows;
	for (int i = 0; i < count; ++i) {
		rows.append(i);
		if (billed.contains(i)) {
			QString from = m_data->data(m_data->index(rows.first().toInt(), 0)).toString();
			QString to = m_data->data(m_data->index(rows.last().toInt(), 0)).toString();
			m_groups->addItem(QString("%1 - %2").arg(from).arg(to), rows);
			rows.clear();
		}
	}
	count = m_groups->count();
	if (count > 0) {
		m_groups->setCurrentIndex(count - 1);
	}
}

/*****************************************************************************/