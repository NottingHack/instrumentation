drop procedure if exists sp_tool_pledged_remain;

DELIMITER //
CREATE PROCEDURE sp_tool_pledged_remain
(
   IN  p_tool_name      varchar(20),
   IN  p_member_id      int,
   OUT p_msg            varchar(200)
)
SQL SECURITY DEFINER
BEGIN
  declare cnt int;
  declare username varchar(50);
  declare seconds_remain int;
  declare hours int;
  declare minutes int;
  declare seconds int;

  set seconds_remain = 0;
  set p_msg = '';

  main: begin

   -- get username
    select m.username
    into username
    from members m
    where m.member_id = p_member_id;

    set p_msg = concat(username, '\n');

    -- Get remaining pledged time
    select -1*sum(tu.usage_duration)
    into seconds_remain
    from tl_tool_usages tu
    inner join tl_tools tl on tl.tool_id = tu.tool_id
    where tu.usage_status ='COMPLETE'
      and tu.member_id = p_member_id
      and tl.tool_name = p_tool_name;

   if (seconds_remain > 0) then
     set p_msg = concat(p_msg, 'remain: ', sec_to_time(seconds_remain));
   end if;



  end main;

END //
DELIMITER ;
