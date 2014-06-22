drop procedure if exists sp_tool_get_status;

/*
  Get details of all tools
*/

DELIMITER //
CREATE PROCEDURE sp_tool_get_status
(
   IN  p_tool_id int
)
SQL SECURITY DEFINER
BEGIN
  main: begin

  select
    t.tool_id,
    t.tool_name,
    t.tool_status,
    t.tool_restrictions,
    t.tool_status_text,
    t.tool_pph,
    ifnull(convert (lst.usage_start , char(40)), '<unknown>') as usage_start,
    ifnull(convert (timestampadd (SECOND, lst.usage_duration, lst.usage_start), char(40)), '<unknown>') as usage_end
  from tl_tools t
  left outer join tl_tool_usages lst on lst.usage_id = (select max(tu.usage_id) from tl_tool_usages tu where tu.tool_id = t.tool_id)
  where t.tool_id = p_tool_id or p_tool_id = -1;
  
  end main;


END //
DELIMITER ;
