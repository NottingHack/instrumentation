drop procedure if exists sp_tool_get_google_info;

DELIMITER //
CREATE PROCEDURE sp_tool_get_google_info
(
   IN  p_id            int,
   OUT p_identity      varchar(255),
   OUT p_refresh_token varchar(255)
)
SQL SECURITY DEFINER
BEGIN
  
  select
    g.identity,
    g.refresh_token
  into
    p_identity,
    p_refresh_token
  from tl_google g
  where (g.id = p_id or p_id is null);
    
END //
DELIMITER ;
