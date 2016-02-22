drop procedure if exists sp_gatekeeper_check_door_access;

/*
  Check if member_id has permission to open door_id.

  Rules:
    1. Doors with a null permission_code can be opened by all current members (member_status = 5)
    2. Doors with a permission_code set can only be opened by people who are in a group with that permission_code (even if they are non-members)

  Sets access_denied:
    0 - access granted
    1 - access denied: non-member (in reality, should mean ex-member, as earlier states shouldn't have a card yet)
    2 - access denied: no permission to open door
    3 - access denied: out of zone - card has been read at an entrance without having signed out last time
   98 - acsses denied: invalid door side (i.e. bug somewhere)
   99 - access denied: other
*/

DELIMITER //
CREATE PROCEDURE sp_gatekeeper_check_door_access
(
   IN  p_member_id      int,
   IN  p_door_id        int,
   IN  p_door_side      varchar(1),
   OUT p_new_zone_id    int,
   OUT p_msg            varchar(100),
   OUT p_access_denied  int
)
SQL SECURITY DEFINER
BEGIN

  declare l_permission_code   varchar(16);
  declare l_member_status     int;
  declare l_access_granted    int;
  declare l_side_a_zone_id    int;
  declare l_side_b_zone_id    int;

  declare l_door_current_zone int; -- Zone on the side of the door where the card was read
  declare l_door_new_zone     int; -- Zone on the opposide side of door_side
  declare l_db_zone_id        int; -- Zone recorded in the database against the member

  declare ck_exists int;

  -- If any data is missing -> access denied
  declare exit handler for not found
  begin
    set p_access_denied = 99;
  end;

  set p_new_zone_id = -1;
  set p_door_side = upper(p_door_side);
  set p_access_denied = 99;
  if p_door_side = '' then
    set p_door_side = NULL;
  end if;

  main: begin

    -- get details of door
    select
      permission_code,
      d.side_a_zone_id,
      d.side_b_zone_id
    into 
      l_permission_code,
      l_side_a_zone_id,
      l_side_b_zone_id
    from doors d
    where d.door_id = p_door_id;

    -- Get zone each side of the door
    if (p_door_side is not null) then
    begin
      if (p_door_side = 'A') then
        set l_door_current_zone = l_side_a_zone_id;
        set l_door_new_zone     = l_side_b_zone_id;
      elseif (p_door_side = 'B') then
        set l_door_current_zone = l_side_b_zone_id;
        set l_door_new_zone     = l_side_a_zone_id;
      else
        -- We've been passed an invalid door side. This shouldn't ever happen...
        set p_access_denied = 98;
        leave main;
      end if;

      set p_new_zone_id = ifnull(l_door_new_zone, -1);

      -- zone_id = 0 is special - it means off-site/outside (i.e. door is an exit). So always allow that, skipping other checks
      if (l_door_new_zone = 0) then
        set p_access_denied = 0;
        leave main;
      end if;
    end;
    end if;


    -- get member_status
    select member_status
    into l_member_status
    from members m
    where m.member_id = p_member_id;
 
    -- If the door has a permission code associated with it, check the member is in a group with that code
    if (l_permission_code is not null) then
      select fn_check_permission(p_member_id, l_permission_code)
      into l_access_granted;

      if (l_access_granted = 0) then
        set p_access_denied = 2;
        leave main;
      end if;
    else
      -- If there's no permission code associated with the door, then the member must be
      -- a current member to open it.
      if (l_member_status != 5) then
        set p_access_denied = 1;
        leave main;
      end if;
    end if;

    -- If no door side has been passed in, then there's no further checks to do
    if (p_door_side is null or p_door_side = '') then
      set p_access_denied = 0;
      leave main;
    end if;

    -- Don't allow entry if the current zone stored against the member doesn't match where the card was read
    -- Get current zone from db
    select count(*) into ck_exists
    from zone_occupancy z
    where z.member_id = p_member_id;

    if (ck_exists = 0) then
      -- no current location recorded for member, so can't possibly be the wrong zone. Allow access.
      set p_access_denied = 0;
      leave main;
    end if;

    select z.zone_id
    into l_db_zone_id
    from zone_occupancy z
    where z.member_id = p_member_id;

    if (l_db_zone_id != l_door_current_zone) then
      set p_access_denied = 3;
      leave main;
    else
      set p_access_denied = 0;
    end if;

  end main;

END //
DELIMITER ;
