drop procedure if exists sp_vend_check_rfid;

/*
  Check an RFID card is known, and linked to an active member.
  Return a vend transaction id if it is, or tran_id=0 if the 
  card isn't valid (and set @err with the reject reason)  
*/

DELIMITER //
CREATE PROCEDURE sp_vend_check_rfid
(
   IN  vmc_id       int,
   IN  rfid_serial  varchar(50),
   OUT tran_id      varchar(6),
   OUT err          varchar(100)
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;
  declare access_granted int;
  declare r_state int;
  declare member_id int;
  
  set tran_id = '0';

  main: begin  
    set access_granted = 0;
    set rfid_serial = rtrim(rfid_serial);
    
    -- First, check the card is suitable (not unknown type)
    if (rfid_serial = 'Unknown Card Type') then
      set err = 'Unknown Card Type';
      set tran_id= '0';
      leave main;
    end if;   

    -- See if the serial is known
    select count(*) into ck_exists
    from members m 
    inner join rfid_tags r on r.member_id = m.member_id 
    where r.rfid_serial = rfid_serial;
      
    if (ck_exists = 0) then
      set err = "RFID serial not found";
      set tran_id= '0';
      leave main;
    end if;

    select 
      m.member_id,
      r.state
    into
      member_id,
      r_state
    from members m 
    inner join rfid_tags r on r.member_id = m.member_id 
    where r.rfid_serial = rfid_serial
    order by state limit 1;

    if (r_state != 10) then -- STATE_ACTIVE
      set err = "RFID serial not active";
      set tran_id= '0';
      leave main;
    end if;

    insert into vend_log(vmc_id, rfid_serial, member_id, enq_datetime) 
    values (vmc_id, rfid_serial, member_id, sysdate());

    set tran_id = last_insert_id();

  end main;
  
  commit;

END //
DELIMITER ;

