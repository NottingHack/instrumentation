drop procedure if exists sp_check_rfid;

/*
  Check an rfid serial is valid, and return the member_id if it is.
*/

DELIMITER //
CREATE PROCEDURE sp_check_rfid
(
   IN  p_rfid_serial  varchar(50),
   OUT p_member_id    int,
   OUT p_err          varchar(100)
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;
  declare r_state int;
  declare member_id int;
  declare member_status int;
  declare access_denied int;

  set p_member_id = NULL;
  set p_err = "";

  main: begin

    -- First, check the card is suitable (not unknown type)
    if (p_rfid_serial = 'Unknown Card Type') then
      set p_err = 'Unknown Card Type';
      leave main;
    end if;

    -- See if the serial is known
    select count(*) into ck_exists
    from members m
    inner join rfid_tags r on r.member_id = m.member_id
    where r.rfid_serial = p_rfid_serial;

    if (ck_exists = 0) then
      set p_err = "Unknown card";
      leave main;
    end if;

    -- Update the last used time set against the card
    update rfid_tags
    set last_used = sysdate()
    where rfid_tags.rfid_serial = p_rfid_serial;

    select
      r.state,
      m.member_id
    into
      r_state,
      p_member_id
    from members m 
    inner join rfid_tags r on r.member_id = m.member_id 
    where r.rfid_serial = p_rfid_serial
    order by state limit 1;

    if (r_state != 10) then -- STATE_ACTIVE
      set p_err = "Inactive card";
      leave main;
    end if;


  end main;

END //
DELIMITER ;
