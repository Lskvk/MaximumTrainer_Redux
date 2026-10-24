#ifndef TABLEVIEWHOVER_H
#define TABLEVIEWHOVER_H

#include <QTableView>
#include <QEvent>
#include <QMouseEvent>
#include <QRubberBand>
#include <QItemSelection>

class TableViewHover : public QTableView
{
    Q_OBJECT

public:
    explicit TableViewHover(QWidget *parent = 0);


private :
    void mouseMoveEvent(QMouseEvent* event);
    void leaveEvent(QEvent *event);
    void focusOutEvent(QFocusEvent* event);
    bool eventFilter(QObject *watched, QEvent *event);


    void selectionChanged (const QItemSelection & selected, const QItemSelection & deselected );


    int curr_row_hovered;
    int curr_row_selected;
    QRubberBand *bandHover;

};

#endif // TABLEVIEWHOVER_H
