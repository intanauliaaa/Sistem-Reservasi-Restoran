#ifndef SERKOM_RESTO_H
#define SERKOM_RESTO_H

#include <iostream>
#include <string>
#include <ctime>
#include <sstream>
#include <iomanip>
#include <mysql/mysql.h>

using namespace std;

// Kelas Induk (Base Class)
class EntitasResto {
protected:
    int id;
    string nama;

public:
    EntitasResto(int id, string nama) : id(id), nama(nama) {}
    virtual ~EntitasResto() {}

    // Method Virtual untuk Polymorphism
    virtual void tampilkanInfo() const {
        cout << "ID: " << id << " | Nama: " << nama << endl;
    }
};

// Kelas Turunan 1 (Inheritance dari EntitasResto)
class MenuItem : public EntitasResto {
private:
    double harga;
    string status;

public:
    MenuItem(int id, string nama, double harga, string status)
        : EntitasResto(id, nama), harga(harga), status(status) {}

    // Polymorphism: Override method dari induk
    void tampilkanInfo() const override {
        cout << "[MENU] " << left << setw(15) << nama << " - Rp " << setw(8) << harga << " [" << status << "]" << endl;
    }
};

// Kelas Turunan 2 (Inheritance dari EntitasResto)
class MejaItem : public EntitasResto {
private:
    int kapasitas;
    string status;

public:
    MejaItem(int id, string nama, int kapasitas, string status)
        : EntitasResto(id, nama), kapasitas(kapasitas), status(status) {}

    // Polymorphism: Override method dari induk
    void tampilkanInfo() const override {
        cout << "[MEJA] Nomor: " << nama << " | Kapasitas: " << kapasitas << " Orang | Status: [" << status << "]" << endl;
    }
};


// Kelas Utama
class serkom_resto {
private:
    MYSQL* conn;

public:
    serkom_resto();
    ~serkom_resto();
    void jalankan();
};

#endif