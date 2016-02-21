drop procedure if exists sp_gatekeeper_check_rfid;

/*
  Check an rfid serial is valid, and return an approprite unlock text if it is.
  Then log an entry in the access log (either granted or denied).
*/

DELIMITER //
CREATE PROCEDURE sp_gatekeeper_check_rfid
(
   IN  p_rfid_serial    varchar(50),
   IN  p_door_id        int,
   OUT p_display_msg    varchar(95),
   OUT p_username       varchar(50),
   OUT p_last_seen      varchar(100),
   OUT p_access_granted int,
   OUT p_err            varchar(100)
)
SQL SECURITY DEFINER
BEGIN
  declare l_member_id int;
  declare l_access_denied int;

  set l_member_id = NULL;

  main: begin
    set p_access_granted = 0;

    -- get member_id from RFID serial
    call sp_check_rfid(p_rfid_serial, l_member_id, p_err);
    if (l_member_id is null) then
      set p_display_msg = concat('Access Denied: ', p_err);
      leave main;
    end if;

    select
      coalesce(m.unlock_text, 'Welcome'),
      coalesce(m.username, '<unknown>')
    into
      p_display_msg,
      p_username
    from members m
    where m.member_id = l_member_id;

    -- Check the card belongs to either a member, or someone with specific access to open the door
    call sp_gatekeeper_check_door_access(l_member_id, p_door_id, l_access_denied);
    if (l_access_denied != 0) then
      -- Vary the message depending on the reason
      if (l_access_denied = 1) then
        set p_err = "Not a current member";
        set p_display_msg = "Access Denied: Ex-member";
      elseif (l_access_denied = 2) then
        set p_err = "No permission to open door";
        set p_display_msg = "Access Denied";
      else
        -- Some other reason, send non-specific Access Denied
        set p_err = "Access Denied (other)";
        set p_display_msg = "Access Denied";
      end if;

      leave main;
    end if;

    set p_access_granted = 1;
  end main;
  
  -- Get last seen text from access log
  call sp_last_seen(l_member_id, p_last_seen);

  -- add entry to access log
  if (p_access_granted = 1) then
    insert into access_log (rfid_serial, pin, access_result, member_id, door_id)
    values (p_rfid_serial, null, 20, l_member_id, p_door_id); -- granted
  else
    insert into access_log (rfid_serial, pin, access_result, member_id, denied_reason, door_id)
    values (p_rfid_serial, null, 10, l_member_id, p_err, p_door_id); -- denied
  end if;

END //
DELIMITER ;
