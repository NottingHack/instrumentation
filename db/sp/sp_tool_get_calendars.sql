drop procedure if exists sp_tool_get_calendars;

/*
  Get details of all tools that should have there bookings published
*/

DELIMITER //
CREATE PROCEDURE sp_tool_get_calendars
(
   IN  p_tool_id int
)
SQL SECURITY DEFINER
BEGIN
  main: begin  

  select
    t.tool_id,
    t.tool_address,
    t.tool_name,
    t.tool_calendar,
    t.tool_cal_poll_ival,
    t.tool_calendar
  from tl_tools t
  where (t.tool_id = p_tool_id or p_tool_id = -1)
    and (t.tool_calendar is not null and length(trim(t.tool_calendar) > 10))
    and (t.tool_cal_poll_ival > 0);

  end main;
 

END //
DELIMITER ;
