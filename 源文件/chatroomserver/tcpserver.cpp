#include "tcpserver.h"

#include <QHostAddress>
#include <QTcpSocket>

TcpServer::TcpServer(QObject *parent) : QObject(parent), m_server(this)
{
    connect(&m_server, &QTcpServer::newConnection, this, &TcpServer::slotNewConnection);
}

TcpServer::~TcpServer()
{
    const QList<TextMessageAssembler*> &al = m_map.values();
    for (auto item : al)
        delete item;

    close();
}

bool TcpServer::listen(quint16 port)
{
    return m_server.listen(QHostAddress::Any, port);
}

void TcpServer::close()
{
    if (m_server.isListening())
        m_server.close();
}

void TcpServer::setHandler(TextMessageHandler *handler)
{
    m_handler = handler;
}

void TcpServer::slotNewConnection()
{
    QTcpSocket *socket = m_server.nextPendingConnection();

    if (socket != nullptr)
    {
        m_map.insert(socket, new TextMessageAssembler);

        connect(socket, &QTcpSocket::readyRead, this, &TcpServer::slotSocketReadyRead);
        connect(socket, &QTcpSocket::disconnected, this, &TcpServer::slotSocketDisconnected);

        if (m_handler != nullptr)
        {
            TextMessgae conn("CONN", socket->peerAddress().toString() + ":" + QString::number(socket->peerPort()));

            m_handler->hanle(*socket, conn);
        }
    }
}

void TcpServer::slotSocketReadyRead()
{
    QTcpSocket *socket = static_cast<QTcpSocket *>(sender());

    TextMessageAssembler *assembler = m_map.value(socket);
    char buf[256] = {0};
    int len = 0;
    while ((len = static_cast<int>(socket->read(buf, sizeof(buf)))) > 0)
    {
        QSharedPointer<TextMessgae> ptm = nullptr;
        assembler->prepare(buf, len);

        while ((ptm = assembler->assemble()) != nullptr)
        {
            if (m_handler != nullptr)
            {
                m_handler->hanle(*socket, *ptm);
            }
        }
    }
}

void TcpServer::slotSocketDisconnected()
{
    QTcpSocket *socket = static_cast<QTcpSocket *>(sender());

    delete m_map.take(socket);

    socket->deleteLater();

    if (m_handler != nullptr)
    {
        TextMessgae conn("DSCN", "");

        m_handler->hanle(*socket, conn);
    }
}
