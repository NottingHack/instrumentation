drop procedure if exists sp_tool_sign_on;

DELIMITER //
CREATE PROCEDURE sp_tool_sign_on
(
   IN  p_tool_name      varchar(20),
   IN  p_rfid_serial    varchar(50),
   OUT p_access_result  int,
   OUT p_msg            varchar(200)
)
SQL SECURITY DEFINER
BEGIN
  declare cnt int;
  declare tool_id int;
  declare username varchar(50);
  declare member_id int;
  declare member_status int;
  declare tool_restrictions varchar(20);
  declare tool_status varchar(20);
  declare access_level varchar(1);

  set p_access_result = 0;
  set p_msg = '';

  main: begin

    -- Check tool name is actaully known
    select count(*)
    into cnt
    from tl_tools t
    where t.tool_name = p_tool_name;

    if (cnt = 0) then
      set p_msg = 'Tool not conf.  ';
    leave main;
    elseif (cnt > 1) then
      set p_msg = 'Tool conf error';
      leave main;
    end if;

    -- Get details
    select 
      t.tool_id,
      t.tool_restrictions,
      t.tool_status
    into 
      tool_id,
      tool_restrictions,
      tool_status
    from tl_tools t
    where t.tool_name = p_tool_name;

    -- Check RFID serial is known
    select count(*)
    into cnt
    from members m
    inner join rfid_tags r on r.member_id = m.member_id
    where r.rfid_serial = p_rfid_serial
      and r.state = 10; -- Active

    if (cnt = 0) then
      set p_msg = 'Unknown card    ';
      leave main;
    elseif (cnt > 1) then -- Error out if multiple active entries for the same RFID card 
      set p_msg = 'Card conf error ';
      leave main;
    end if;

    -- Get member details from card
    select 
      m.username,
      m.member_id,
      m.member_status
    into 
      username,
      member_id,
      member_status
    from members m
    inner join rfid_tags r on r.member_id = m.member_id
    where r.rfid_serial = p_rfid_serial
      and r.state = 10; -- Active

    -- Check member is current
    if (member_status != 5) then
      set p_msg = 'Not a member    ';
      leave main;
    end if;

    -- If the tool isn't restircted, grant access now
    if (tool_restrictions = 'UNRESTRICTED') then
      set p_access_result = 1;
      set p_msg = 'U'; -- User.
      leave main;
    end if;

    -- Tool is restricted, so check member has been inducted
    select count(*)
    into cnt
    from tl_members_tools mt
    where mt.member_id = member_id
      and mt.tool_id   = tool_id;

    if (cnt != 1) then
      set p_msg = 'Not inducted';
      leave main;
    end if;

    -- get access level - user, inductor or maintainer
    select 
      case mt.mt_access_level
        when 'INDUCTOR'   then 'I'
        when 'MAINTAINER' then 'M'
        else                   'U' -- User
      end as al
    into access_level
    from tl_members_tools mt
    where mt.member_id = member_id
      and mt.tool_id   = tool_id;

    -- Check tool hasn't been disabled
    if (tool_status = 'DISABLED') then
      set p_msg = 'Out of service';
      leave main;
    end if;

    -- Ok, now grant access
    set p_msg = access_level;
    set p_access_result = 1;

  end main;

  -- Add use entry
  if (p_access_result = 1) then
    insert into tl_tool_usages (member_id, tool_id, usage_start, usage_status )
                        values (member_id, tool_id, sysdate()  , 'IN_PROGRESS');
  end if;

  -- Update tool status to in use
  update tl_tools 
  set tool_status = 'IN_USE'
  where tl_tools.tool_id = tool_id;

END //
DELIMITER ;
