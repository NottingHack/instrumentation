drop procedure if exists sp_rfid_update;

/*
   Deal with the transition between RFID serials being stored as the last 4-bytes 
   converted to a long then put in a varchar, to storing the full serial (4, 7, 
   or 10 bytes). The "old" serials are kept in rfid_serial_legacy, whilst the 
   full serial (when known) is in rfid_serial.
*/

DELIMITER //
CREATE PROCEDURE sp_rfid_update
(
   IN  p_rfid_serial        varchar(50),
   IN  p_rfid_serial_legacy varchar(50),
   OUT p_msg                varchar(100)
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;

  set p_msg = '';

  main: begin

    -- First, check the card is suitable (not unknown type)
    if (p_rfid_serial = 'Unknown Card Type') then
      set p_msg = 'Unknown Card Type';
      leave main;
    end if;

    -- See if the serial is known
    select count(*) into ck_exists
    from rfid_tags r
    where r.rfid_serial = p_rfid_serial;

    if (ck_exists = 1) then
      set p_msg = 'card found';
      leave main;
    end if;

    -- card not found using full UID
    -- see if there is a match searching by legacy (4 byte as a long) format
    select count(*) into ck_exists
    from rfid_tags r
    where r.rfid_serial_legacy = p_rfid_serial_legacy
      and r.rfid_serial is NULL;

    if (ck_exists = 1) then
      update rfid_tags
      set rfid_serial = p_rfid_serial
      where rfid_serial_legacy = p_rfid_serial_legacy;

      set p_msg = 'Record updated';
      leave main;
    end if;

    set p_msg = 'card not found';


  end main;

END //
DELIMITER ;
