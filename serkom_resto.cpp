#include "serkom_resto.h"

serkom_resto::serkom_resto() {
    conn = mysql_init(NULL);
    if (conn == NULL) {
        cout << "[ERROR] Inisialisasi MySQL gagal!" << endl;
        return;
    }

    my_bool ssl_enforce = 0;
    mysql_options(conn, MYSQL_OPT_SSL_ENFORCE, &ssl_enforce);
    my_bool verify = 0;
    mysql_options(conn, MYSQL_OPT_SSL_VERIFY_SERVER_CERT, &verify);
    mysql_ssl_set(conn, NULL, NULL, NULL, NULL, NULL);

    MYSQL* connect_result = mysql_real_connect(conn, "localhost", "root", "", "serkom_resto", 3306, NULL, 0);

    if (!connect_result) {
        cout << "[ERROR] Gagal terhubung ke database: " << mysql_error(conn) << endl;
    }
}

serkom_resto::~serkom_resto() {
    if (conn) {
        mysql_close(conn);
        cout << "\n[INFO] Koneksi database ditutup. Terima kasih telah menggunakan aplikasi!" << endl;
    }
}

void serkom_resto::jalankan() {
    if (!conn) {
        cout << "Program dihentikan karena gagal terhubung ke database." << endl;
        return;
    }

    int pilihan;
    do {
        cout << "\n=========================================" << endl;
        cout << "               SERKOM RESTO              " << endl;
        cout << "=========================================" << endl;
        cout << "1. Lihat Daftar Menu" << endl;
        cout << "2. Tambah Pelanggan & Reservasi" << endl;
        cout << "3. Lihat Daftar Reservasi" << endl;
        cout << "4. Ubah Status Menu (Tersedia/Habis)" << endl;
        cout << "5. Ubah Status Meja (Tersedia/Terisi)" << endl;
        cout << "6. Hapus Histori Reservasi (by ID TRX)" << endl;
        cout << "7. Keluar" << endl;
        cout << "Pilih menu (1-7): ";
        
        if (!(cin >> pilihan)) {
            cout << "\n[GAGAL] Masukkan angka 1 sampai 7!\n";
            cin.clear();
            cin.ignore(10000, '\n');
            continue;
        }

        cin.ignore();

        switch (pilihan) {
            case 1: {
                cout << "\n--- DAFTAR MENU RESTORAN ---\n" << endl;
                string query = "SELECT nama_menu, harga, kategori, status_menu FROM menu ORDER BY kategori DESC, nama_menu ASC";
                
                if (mysql_query(conn, query.c_str())) {
                    cout << "[ERROR] Gagal mengambil data menu: " << mysql_error(conn) << endl;
                    break;
                }

                MYSQL_RES* result = mysql_store_result(conn);
                if (result == NULL) {
                    cout << "[ERROR] Gagal menyimpan hasil query: " << mysql_error(conn) << endl;
                    break;
                }

                MYSQL_ROW row;
                string current_kategori = "";
                int nomor = 1;

                while ((row = mysql_fetch_row(result))) {
                    string nama = row[0];
                    string harga = row[1];
                    string kategori = row[2];
                    string status = row[3];

                    if (kategori != current_kategori) {
                        current_kategori = kategori;
                        cout << "\n[" << current_kategori << "]" << endl;
                        cout << "--------------------------------------------------" << endl;
                        nomor = 1;
                    }

                    cout << nomor << ". " << left << setw(15) << nama << " - Rp " << setw(8) << harga << " [" << status << "]" << endl;
                    nomor++;
                }

                mysql_free_result(result);
                cout << "--------------------------------------------------" << endl;
                break;
            }
            case 2: {
                auto t = time(nullptr);
                auto tm = *localtime(&t);
                ostringstream oss;
                oss << put_time(&tm, "%Y-%m-%d %H:%M:%S");
                string tanggal = oss.str();

                cout << "\n=== FORM TAMBAH RESERVASI & PESANAN ===" << endl;
                cout << "Waktu Reservasi (Auto)    : " << tanggal << endl;

                string nama;
                int nomor_meja;
                cout << "Masukkan Nama Pemesan     : ";
                getline(cin, nama);
                cout << "Masukkan Nomor Meja (1-20): ";
                
                if (!(cin >> nomor_meja) || nomor_meja < 1 || nomor_meja > 20) {
                    cout << "Nomor meja tersedia 1 sampai 20.\n";
                    cin.clear();
                    cin.ignore(10000, '\n');
                    break;
                }
                cin.ignore();

                string q_cek_meja = "SELECT id_meja, status_meja FROM meja WHERE nomor_meja = " + to_string(nomor_meja);
                mysql_query(conn, q_cek_meja.c_str());
                MYSQL_RES* res_meja = mysql_store_result(conn);
                MYSQL_ROW row_meja = mysql_fetch_row(res_meja);
                
                long long id_meja = 0;
                if (row_meja) {
                    id_meja = stoll(row_meja[0]);
                    string status_meja = row_meja[1];
                    if (status_meja == "Terisi") {
                        cout << "\n[GAGAL] Mohon maaf, Meja Nomor " << nomor_meja << " saat ini sedang PENUH / TERISI!" << endl;
                        mysql_free_result(res_meja);
                        break;
                    }
                } else {
                    cout << "\n[GAGAL] Nomor meja tidak terdaftar di sistem!" << endl;
                    mysql_free_result(res_meja);
                    break;
                }
                mysql_free_result(res_meja);

                string q_update_meja = "UPDATE meja SET status_meja = 'Terisi' WHERE id_meja = " + to_string(id_meja);
                mysql_query(conn, q_update_meja.c_str());

                string q_pelanggan = "INSERT INTO pelanggan (nama) VALUES ('" + nama + "')";
                if (mysql_query(conn, q_pelanggan.c_str())) {
                    cout << "[ERROR] Gagal menyimpan data pelanggan: " << mysql_error(conn) << endl;
                    break;
                }
                long long id_pelanggan = mysql_insert_id(conn);

                string q_reservasi = "INSERT INTO reservasi (id_pelanggan, id_meja, waktu_reservasi) VALUES (" + 
                                     to_string(id_pelanggan) + ", " + to_string(id_meja) + ", '" + tanggal + "')";
                if (mysql_query(conn, q_reservasi.c_str())) {
                    cout << "[ERROR] Gagal membuat reservasi: " << mysql_error(conn) << endl;
                    break;
                }
                long long id_reservasi = mysql_insert_id(conn);

                string q_hitung = "SELECT COUNT(*) FROM reservasi WHERE DATE(waktu_reservasi) = DATE('" + tanggal + "')";
                mysql_query(conn, q_hitung.c_str());
                MYSQL_RES* res_hitung = mysql_store_result(conn);
                MYSQL_ROW row_h = mysql_fetch_row(res_hitung);
                int nomor_pesanan_harian = 1;
                if (row_h) {
                    nomor_pesanan_harian = stoi(row_h[0]);
                }
                mysql_free_result(res_hitung);

                cout << "\n--- DAFTAR MENU ---" << endl;
                mysql_query(conn, "SELECT id_menu, nama_menu, harga, status_menu FROM menu");
                MYSQL_RES* res_menu = mysql_store_result(conn);
                MYSQL_ROW row_m;
                while ((row_m = mysql_fetch_row(res_menu))) {
                    cout << "ID [" << row_m[0] << "] " << left << setw(15) << row_m[1] << " - Rp " << setw(8) << row_m[2] << " [" << row_m[3] << "]" << endl;
                }
                mysql_free_result(res_menu);

                char lanjut = 'y';
                while (lanjut == 'y' || lanjut == 'Y') {
                    int id_menu, jumlah;
                    cout << "\nMasukkan ID Menu yang dipesan : ";
                    
                    if (!(cin >> id_menu)) {
                        cout << "[GAGAL] ID Menu harus berupa angka!" << endl;
                        cin.clear();
                        cin.ignore(10000, '\n');
                        continue;
                    }
                    cin.ignore();

                    string q_cek_menu = "SELECT nama_menu, status_menu FROM menu WHERE id_menu = " + to_string(id_menu);
                    if (mysql_query(conn, q_cek_menu.c_str())) {
                        cout << "[ERROR] Gagal memeriksa menu.\n";
                        continue;
                    }
                    MYSQL_RES* res_m_cek = mysql_store_result(conn);
                    MYSQL_ROW row_m_cek = mysql_fetch_row(res_m_cek);

                    if (!row_m_cek) {
                        cout << "[GAGAL] ID Menu tidak ditemukan!" << endl;
                        mysql_free_result(res_m_cek);
                        continue;
                    }

                    string nama_menu_pilih = row_m_cek[0];
                    string status_menu_pilih = row_m_cek[1];
                    mysql_free_result(res_m_cek);

                    if (status_menu_pilih == "Habis") {
                        cout << "[GAGAL] Mohon maaf, menu \"" << nama_menu_pilih << "\" sedang HABIS! Silakan pilih menu lain." << endl;
                        continue;
                    }

                    cout << "Masukkan Jumlah Porsi/Gelas : ";
                    if (!(cin >> jumlah)) {
                        cout << "[GAGAL] Jumlah harus berupa angka!\n";
                        cin.clear();
                        cin.ignore(10000, '\n');
                        continue;
                    }
                    cin.ignore();

                    string q_detail = "INSERT INTO detail_reservasi (id_reservasi, id_menu, jumlah) VALUES (" +
                                      to_string(id_reservasi) + ", " + to_string(id_menu) + ", " + to_string(jumlah) + ")";
                    if (mysql_query(conn, q_detail.c_str())) {
                        cout << "[ERROR] Gagal memesan menu: " << mysql_error(conn) << endl;
                    } else {
                        cout << "[SUKSES] " << nama_menu_pilih << " berhasil ditambahkan ke pesanan." << endl;
                    }

                    cout << "Tambah menu lain? (y/n): ";
                    cin >> lanjut;
                    cin.ignore();
                }

                cout << "\n=========================================" << endl;
                cout << "           STRUK PEMESANAN RESTORAN      " << endl;
                cout << "=========================================" << endl;
                cout << "Tanggal          : " << tanggal << endl;
                cout << "Nomor Pesanan    : #" << nomor_pesanan_harian << " (ID Transaksi: " << id_reservasi << ")" << endl;
                cout << "Nama Pemesan     : " << nama << endl;
                cout << "Nomor Meja       : " << nomor_meja << endl;
                cout << "-----------------------------------------" << endl;
                cout << "DAFTAR PESANAN:" << endl;

                string q_struk = "SELECT m.nama_menu, d.jumlah, m.harga, (d.jumlah * m.harga) as subtotal "
                                 "FROM detail_reservasi d JOIN menu m ON d.id_menu = m.id_menu "
                                 "WHERE d.id_reservasi = " + to_string(id_reservasi);
                mysql_query(conn, q_struk.c_str());
                MYSQL_RES* res_struk = mysql_store_result(conn);
                MYSQL_ROW r_struk;
                int no_item = 1;
                double total_harga = 0;

                while ((r_struk = mysql_fetch_row(res_struk))) {
                    double sub = stod(r_struk[3]);
                    total_harga += sub;
                    cout << no_item++ << ". " << left << setw(15) << r_struk[0] << " (x" << r_struk[1] << ") \t- Rp " << r_struk[3] << endl;
                }
                mysql_free_result(res_struk);

                cout << "-----------------------------------------" << endl;
                cout << "TOTAL HARGA      : Rp " << total_harga << endl;
                cout << "=========================================" << endl;
                cout << "            Selamat Menikmati            " << endl;
                cout << "=========================================" << endl;
                break;
            }
            case 3: {
                cout << "\n--- DAFTAR / HISTORI RESERVASI ---\n" << endl;
                string q_histori = "SELECT r.id_reservasi, r.waktu_reservasi, p.nama, m.nomor_meja, "
                                   "COALESCE(SUM(d.jumlah * mn.harga), 0) as total_biaya "
                                   "FROM reservasi r "
                                   "JOIN pelanggan p ON r.id_pelanggan = p.id_pelanggan "
                                   "JOIN meja m ON r.id_meja = m.id_meja "
                                   "LEFT JOIN detail_reservasi d ON r.id_reservasi = d.id_reservasi "
                                   "LEFT JOIN menu mn ON d.id_menu = mn.id_menu "
                                   "GROUP BY r.id_reservasi, r.waktu_reservasi, p.nama, m.nomor_meja "
                                   "ORDER BY r.waktu_reservasi DESC";

                if (mysql_query(conn, q_histori.c_str())) {
                    cout << "[ERROR] Gagal mengambil histori reservasi: " << mysql_error(conn) << endl;
                    break;
                }

                MYSQL_RES* res_hist = mysql_store_result(conn);
                if (res_hist == NULL) {
                    cout << "[ERROR] Gagal menyimpan hasil query: " << mysql_error(conn) << endl;
                    break;
                }

                MYSQL_ROW row_h;
                int nomor = 1;
                cout << "No | ID TRX | Waktu Reservasi     | Nama Pemesan   | Meja | Total Biaya" << endl;
                cout << "------------------------------------------------------------------" << endl;

                while ((row_h = mysql_fetch_row(res_hist))) {
                    string id_res = row_h[0];
                    string waktu = row_h[1];
                    string nama_p = row_h[2];
                    string no_meja = row_h[3];
                    string total = row_h[4];

                    cout << left << setw(2) << nomor++ << " | #" 
                         << left << setw(5) << id_res << " | " 
                         << left << setw(19) << waktu << " | " 
                         << left << setw(14) << nama_p << " | " 
                         << left << setw(4) << no_meja << " | Rp " << total << endl;
                }

                mysql_free_result(res_hist);
                cout << "------------------------------------------------------------------" << endl;
                break;
            }
            case 4: {
                cout << "\n=== UBAH STATUS MENU (Tersedia / Habis) ===" << endl;
                
                string q_menu = "SELECT id_menu, nama_menu, status_menu FROM menu";
                if (mysql_query(conn, q_menu.c_str())) {
                    cout << "[ERROR] Gagal mengambil data menu.\n";
                    break;
                }
                MYSQL_RES* res_m = mysql_store_result(conn);
                MYSQL_ROW row_m;
                
                cout << left << setw(4) << "ID" << " | " << setw(18) << "Nama Menu" << " | " << "Status Saat Ini" << endl;
                cout << "--------------------------------------------" << endl;
                while ((row_m = mysql_fetch_row(res_m))) {
                    cout << left << setw(4) << row_m[0] << " | " 
                         << left << setw(18) << row_m[1] << " | [" 
                         << row_m[2] << "]" << endl;
                }
                mysql_free_result(res_m);
                cout << "--------------------------------------------" << endl;

                int id_target;
                cout << "Masukkan ID Menu yang ingin diubah statusnya: ";
                if (!(cin >> id_target)) {
                    cout << "[GAGAL] ID harus berupa angka!\n";
                    cin.clear();
                    cin.ignore(10000, '\n');
                    break;
                }
                cin.ignore();

                string q_check = "SELECT status_menu FROM menu WHERE id_menu = " + to_string(id_target);
                mysql_query(conn, q_check.c_str());
                MYSQL_RES* res_chk = mysql_store_result(conn);
                MYSQL_ROW row_chk = mysql_fetch_row(res_chk);

                if (!row_chk) {
                    cout << "[GAGAL] ID Menu tidak ditemukan!\n";
                    mysql_free_result(res_chk);
                    break;
                }

                string current_status = row_chk[0];
                mysql_free_result(res_chk);

                string new_status = (current_status == "Tersedia") ? "Habis" : "Tersedia";
                string q_update = "UPDATE menu SET status_menu = '" + new_status + "' WHERE id_menu = " + to_string(id_target);
                
                if (mysql_query(conn, q_update.c_str())) {
                    cout << "[ERROR] Gagal mengubah status menu: " << mysql_error(conn) << endl;
                } else {
                    cout << "[SUKSES] Status menu ID " << id_target << " berhasil diubah menjadi: " << new_status << endl;
                }
                break;
            }
            case 5: {
                cout << "\n=== UBAH STATUS MEJA (Tersedia / Terisi) ===" << endl;
                
                string q_meja = "SELECT id_meja, nomor_meja, kapasitas, status_meja FROM meja ORDER BY nomor_meja ASC";
                if (mysql_query(conn, q_meja.c_str())) {
                    cout << "[ERROR] Gagal mengambil data meja.\n";
                    break;
                }
                MYSQL_RES* res_mj = mysql_store_result(conn);
                MYSQL_ROW row_mj;
                
                cout << left << setw(4) << "ID" << " | " << setw(10) << "No Meja" << " | " << setw(10) << "Kapasitas" << " | " << "Status Saat Ini" << endl;
                cout << "--------------------------------------------------------" << endl;
                while ((row_mj = mysql_fetch_row(res_mj))) {
                    cout << left << setw(4) << row_mj[0] << " | " 
                         << left << setw(10) << row_mj[1] << " | " 
                         << left << setw(10) << row_mj[2] << " | [" 
                         << row_mj[3] << "]" << endl;
                }
                mysql_free_result(res_mj);
                cout << "--------------------------------------------------------" << endl;

                int id_m_target;
                cout << "Masukkan ID Meja yang ingin diubah statusnya: ";
                if (!(cin >> id_m_target)) {
                    cout << "[GAGAL] ID harus berupa angka!\n";
                    cin.clear();
                    cin.ignore(10000, '\n');
                    break;
                }
                cin.ignore();

                string q_check_m = "SELECT status_meja, nomor_meja FROM meja WHERE id_meja = " + to_string(id_m_target);
                mysql_query(conn, q_check_m.c_str());
                MYSQL_RES* res_chk_m = mysql_store_result(conn);
                MYSQL_ROW row_chk_m = mysql_fetch_row(res_chk_m);

                if (!row_chk_m) {
                    cout << "[GAGAL] ID Meja tidak ditemukan!\n";
                    mysql_free_result(res_chk_m);
                    break;
                }

                string current_status_m = row_chk_m[0];
                string no_meja_val = row_chk_m[1];
                mysql_free_result(res_chk_m);

                string new_status_m = (current_status_m == "Tersedia") ? "Terisi" : "Tersedia";
                string q_update_m = "UPDATE meja SET status_meja = '" + new_status_m + "' WHERE id_meja = " + to_string(id_m_target);
                
                if (mysql_query(conn, q_update_m.c_str())) {
                    cout << "[ERROR] Gagal mengubah status meja: " << mysql_error(conn) << endl;
                } else {
                    cout << "[SUKSES] Status Meja Nomor " << no_meja_val << " berhasil diubah menjadi: " << new_status_m << endl;
                }
                break;
            }
            case 6: {
                cout << "\n=== HAPUS HISTORI RESERVASI ===" << endl;
                int id_del;
                cout << "Masukkan ID Transaksi (TRX) yang ingin dihapus: ";
                
                if (!(cin >> id_del)) {
                    cout << "[GAGAL] ID TRX harus berupa angka!\n";
                    cin.clear();
                    cin.ignore(10000, '\n');
                    break;
                }
                cin.ignore();

                string q_check_del = "SELECT id_meja FROM reservasi WHERE id_reservasi = " + to_string(id_del);
                mysql_query(conn, q_check_del.c_str());
                MYSQL_RES* res_cdel = mysql_store_result(conn);
                MYSQL_ROW row_cdel = mysql_fetch_row(res_cdel);

                if (!row_cdel) {
                    cout << "[GAGAL] ID Transaksi #" << id_del << " tidak ditemukan!" << endl;
                    mysql_free_result(res_cdel);
                    break;
                }

                string id_meja_terikat = row_cdel[0];
                mysql_free_result(res_cdel);

                string q_del_detail = "DELETE FROM detail_reservasi WHERE id_reservasi = " + to_string(id_del);
                mysql_query(conn, q_del_detail.c_str());

                string q_del_main = "DELETE FROM reservasi WHERE id_reservasi = " + to_string(id_del);
                if (mysql_query(conn, q_del_main.c_str())) {
                    cout << "[ERROR] Gagal menghapus reservasi: " << mysql_error(conn) << endl;
                } else {
                    string q_free_meja = "UPDATE meja SET status_meja = 'Tersedia' WHERE id_meja = " + id_meja_terikat;
                    mysql_query(conn, q_free_meja.c_str());

                    cout << "[SUKSES] Transaksi #" << id_del << " berhasil dihapus dan Meja telah diset Tersedia kembali!" << endl;
                }
                break;
            }
            case 7:
                cout << "\nKeluar dari program...\n";
                break;
            default:
                cout << "\nPilihan tidak valid! Silakan masukkan angka 1-7.\n";
        }
    } while (pilihan != 7);
}