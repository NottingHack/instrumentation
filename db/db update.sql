
ALTER TABLE rfid_tags ADD rfid_serial_legacy VARCHAR(50) AFTER rfid_serial;

ALTER TABLE rfid_tags MODIFY rfid_serial VARCHAR(50);

ALTER TABLE rfid_tags DROP PRIMARY KEY;

ALTER TABLE rfid_tags ADD rfid_id INT PRIMARY KEY AUTO_INCREMENT FIRST;

ALTER TABLE rfid_tags ADD constraint product_rfid_serial unique (rfid_serial);
ALTER TABLE rfid_tags ADD constraint product_rfid_serial_legacy unique (rfid_serial_legacy);

update rfid_tags set rfid_serial_legacy = rfid_serial;
update rfid_tags set rfid_serial = null;

