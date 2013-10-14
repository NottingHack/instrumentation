drop procedure if exists sp_get_details_from_rfid;

/*
  Lookup handle and balance from RFID serial - used after
  card is first presented to vending machine
*/

DELIMITER //
CREATE PROCEDURE sp_get_details_from_rfid
(
   IN  rfid_serial  varchar(50),
   OUT username     varchar(100),
   OUT balance      int,
   OUT err          varchar(100)
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;
  declare member_id int;

  main: begin  
    set rfid_serial = rtrim(rfid_serial);

    -- See if the serial is known
    select count(*) into ck_exists
    from members m 
    inner join rfid_tags r on r.member_id = m.member_id 
    where r.rfid_serial = rfid_serial;
      
    if (ck_exists = 0) then
      set err = "RFID serial not found";
      leave main;
    end if;

    select 
      m.username,
      m.balance
    into
      username,
      balance
    from members m 
    inner join rfid_tags r on r.member_id = m.member_id 
    where r.state = 10 -- STATE_ACTIVE
      and r.rfid_serial = rfid_serial
    order by state limit 1;

  end main;

END //
DELIMITER ;
