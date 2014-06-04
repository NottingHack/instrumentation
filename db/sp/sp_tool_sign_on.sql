drop procedure if exists sp_tool_sign_on;

DELIMITER //
CREATE PROCEDURE sp_tool_sign_on
(
   IN  p_tool_name      varchar(20),
   IN  p_rfid_serial    varchar(50),
   OUT p_access_result  int,
   OUT p_msg            varchar(200),
   OUT p_member_id      int
)
SQL SECURITY DEFINER
BEGIN
  declare cnt int;
  declare tool_id int;
  declare username varchar(50);
  declare member_status int;
  declare tool_restrictions varchar(20);
  declare tool_status varchar(20);
  declare access_level varchar(1);
  declare tool_pph int;
  declare credit_limit int;
  declare balance int;

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
      t.tool_status,
      t.tool_pph
    into 
      tool_id,
      tool_restrictions,
      tool_status,
      tool_pph
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
      m.member_status,
      m.balance,
      m.credit_limit
    into 
      username,
      p_member_id,
      member_status,
      balance,
      credit_limit
    from members m
    inner join rfid_tags r on r.member_id = m.member_id
    where r.rfid_serial = p_rfid_serial
      and r.state = 10; -- Active

    -- Check member is current
    if (member_status != 5) then
      set p_msg = 'Not a member    ';
      leave main;
    end if;

    -- Check tool hasn't been disabled
    if (tool_status = 'DISABLED') then
      set p_msg = 'Out of service';
      leave main;
    end if;
    
    -- If a charge applies for using the tool, check the member has some credit
    if (tool_pph > 0) then

      -- First check balance vs credit limit
      if (balance <= -1*credit_limit) then
        -- Member is over their credit limit, but they may still have pledged time to
        -- use up, so check for that
        if 
          (
            select ifnull(sum(tu.usage_duration), 0) -- get the total pledged time remaining (e.g. -60 here means 1min of pledged time left)
            from tl_tool_usages tu
            where tu.member_id = p_member_id
              and tu.tool_id = tool_id
              and tu.usage_status = 'COMPLETE' 
          ) >= 0 then
          -- Over credit limit, and no pledged time remaing.
            set p_msg = 'Out of credit';
            leave main;
          end if;
      end if;
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
    where mt.member_id = p_member_id
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
    where mt.member_id = p_member_id
      and mt.tool_id   = tool_id;

    -- Ok, now grant access
    set p_msg = access_level;
    set p_access_result = 1;

  end main;

  -- Add use entry
  if (p_access_result = 1) then
    insert into tl_tool_usages (member_id, tool_id, usage_start, usage_status )
                        values (p_member_id, tool_id, sysdate()  , 'IN_PROGRESS');

    -- Update tool status to in use
    update tl_tools 
    set tool_status = 'IN_USE'
    where tl_tools.tool_id = tool_id;
  end if;

END //
DELIMITER ;
