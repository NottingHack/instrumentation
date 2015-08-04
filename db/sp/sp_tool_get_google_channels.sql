drop procedure if exists sp_tool_get_google_channels;

DELIMITER //
CREATE PROCEDURE sp_tool_get_google_channels
(
   IN  p_tool_id int
)
SQL SECURITY DEFINER
BEGIN
  
  select
    g.channel_id,
    g.tool_id,
    g.channel_token,
    g.resource_id,
    g.channel_created,
    g.channel_expiration
  from tl_google_notifications g
  where g.tool_id = p_tool_id;
    
END //
DELIMITER ;
