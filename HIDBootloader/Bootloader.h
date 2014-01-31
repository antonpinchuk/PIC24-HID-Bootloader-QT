#ifndef BOOTLOADER_H
#define BOOTLOADER_H

#include "Comm.h"

/*!
 * Provides HID bootloader communication.
 */
class Bootloader : public QObject {
    Q_OBJECT

signals:

protected:

public:

    explicit Bootloader();
    ~Bootloader();

};

#endif // BOOTLOADER_H
