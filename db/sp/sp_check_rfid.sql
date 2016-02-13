drop procedure if exists sp_check_rfid;

/*
  Check an rfid serial is valid, and return an approprite unlock text if it is.
  Then log an entry in the access log (either granted or denied).
*/

DELIMITER //
CREATE PROCEDURE sp_check_rfid
(
   IN  rfid_serial    varchar(50),
   IN  door_id        int,
   OUT display_msg    varchar(95),
   OUT username       varchar(50),
   OUT last_seen      varchar(100),
   OUT access_granted int,
   OUT err            varchar(100)
)
SQL SECURITY DEFINER
BEGIN
  declare ck_exists int;
  declare r_state int;
  declare member_id int;
  declare member_status int;
  declare access_denied int;

  set member_id = NULL;
  
  main: begin  
    set access_granted = 0;
    
    -- First, check the card is suitable (not unknown type)
    if (rfid_serial = 'Unknown Card Type') then
      set err = 'Unknown Card Type';
      set display_msg = 'Unknown Card Type';
      leave main;
    end if;   

    -- See if the serial is known
    select count(*) into ck_exists
    from members m 
    inner join rfid_tags r on r.member_id = m.member_id 
    where r.rfid_serial = rfid_serial;
      
    if (ck_exists = 0) then
      set err = "RFID serial not found";
      set display_msg = "Access Denied: Unknown card";
      leave main;
    end if;
    
    -- Update the last used time set against the card
    update rfid_tags 
    set last_used = sysdate()
    where rfid_tags.rfid_serial = rfid_serial;

    select 
      m.member_id,
      coalesce(m.unlock_text, 'Welcome'),
      coalesce(m.username, '<unknown>'),
      r.state,
      m.member_status
    into
      member_id,
      display_msg,
      username,
      r_state,
      member_status
    from members m 
    inner join rfid_tags r on r.member_id = m.member_id 
    where r.rfid_serial = rfid_serial
    order by state limit 1;

    if (r_state != 10) then -- STATE_ACTIVE
      set err = "RFID serial not active";
      set display_msg = "Access Denied: Inactive card";
      leave main;
    end if;

    -- Check the card belongs to either a member, or someone with specific access to open the door
    call sp_gatekeeper_check_access(member_id, door_id, access_denied);
    if (access_denied != 0) then
      -- Vary the message depending on the reason
      if (access_denied = 1) then
        set err = "Not a current member";
        set display_msg = "Access Denied: Ex-member";
      elseif (access_denied = 2) then
        set err = "No permission to open door";
        set display_msg = "Access Denied";
      else
        -- Some other reason, send non-specific Access Denied
        set err = "Access Denied (other)";
        set display_msg = "Access Denied";
      end if;

      leave main;
    end if;

    set access_granted = 1;
  end main;
  
  -- Get last seen text from access log
  call sp_last_seen(member_id, last_seen);

  -- add entry to access log
  if (access_granted = 1) then
    insert into access_log (rfid_serial, pin, access_result, member_id, door_id)
    values (rfid_serial, null, 20, member_id, door_id); -- granted
  else
    insert into access_log (rfid_serial, pin, access_result, member_id, denied_reason, door_id)
    values (rfid_serial, null, 10, member_id, err, door_id); -- denied
  end if;

END //
DELIMITER ;
