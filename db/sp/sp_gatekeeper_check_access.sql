drop procedure if exists sp_gatekeeper_check_access;

/*
  Check if member_id has permission to open door_id.

  Rules:
    1. Doors with a null permission_code can be opened by all current members (member_status = 5)
    2. Doors with a permission_code set can only be opened by people who are in a group with that permission_code (even if they are non-members)

  Sets access_denied:
    0 - access granted
    1 - access denied: non-member (in reality, should mean ex-member, as earlier states shouldn't have a card yet)
    2 - access denied: no permission to open door
   99 - access denied: other
*/

DELIMITER //
CREATE PROCEDURE sp_gatekeeper_check_access
(
   IN  p_member_id      int,
   IN  p_door_id        int,
   OUT p_access_denied  int
)
SQL SECURITY DEFINER
BEGIN

  declare l_permission_code varchar(16);
  declare l_member_status   int;
  declare l_access_granted  int;
  
  -- If any data is missing -> access denied
  declare exit handler for not found
  begin
    set p_access_denied = 99;
  end;

  set p_access_denied = 99;

  main: begin

    -- get permission_code
    select permission_code
    into l_permission_code
    from doors d
    where d.door_id = p_door_id;

    -- get member_status
    select member_status
    into l_member_status
    from members m
    where m.member_id = p_member_id;
    
    -- 1. Doors with a null permission_code can be opened by all current members (member_status = 5)
    if (l_permission_code is null and l_member_status = 5) then
      -- current member & non-restricted door -> access granted
      set p_access_denied = 0;
      leave main;
    end if;
  
    if (l_permission_code is null) then
      -- no permission code is set and the user isn't a current member -> access denied
      set p_access_denied = 1;
      leave main;
    end if;

    -- Check if the member is in a group with the relevant permission code
    select fn_check_permission(p_member_id, l_permission_code)
    into l_access_granted;
    
    if (l_access_granted = 1) then
      set p_access_denied = 0;
    else
      set p_access_denied = 2;
    end if;

  end main;


END //
DELIMITER ;
