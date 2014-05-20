drop procedure if exists sp_tool_sign_off;

DELIMITER //
CREATE PROCEDURE sp_tool_sign_off
(
   IN  p_tool_name          varchar(20),
   IN  p_usage_active_time  int,
   OUT p_msg                varchar(200)
)
SQL SECURITY DEFINER
BEGIN
  
  declare cnt int;
  declare tool_status varchar(20);
  declare done int default false;
  declare usage_id int;

  declare tool_use_cur cursor for 
    select tu.usage_id
    from tl_tool_usages tu
    inner join tl_tools t on t.tool_id = tu.tool_id
    where tu.usage_status = 'IN_PROGRESS'
      and t.tool_name = p_tool_name;
  
  declare continue handler for not found set done = TRUE;

  set p_msg = '';
  
  main: begin
   
    -- Check tool name is actaully known
    select count(*)
    into cnt
    from tl_tools t
    where t.tool_name = p_tool_name;
    
    if (cnt = 0) then
      set p_msg = 'Tool not configured';
    leave main;
    elseif (cnt > 1) then
      set p_msg = 'Tool config error';
      leave main;
    end if;
    
    -- Get tool status
    select 
      t.tool_status
    into 
      tool_status
    from tl_tools t
    where t.tool_name = p_tool_name;  
    
    -- Set the tool's status back to free if applicable
    if (tool_status = 'IN_USE') then
      update tl_tools
      set tool_status = 'FREE'
      where tl_tools.tool_id = tool_id;
    end if;  
    
    open tool_use_cur;
    
    read_loop: LOOP
      fetch tool_use_cur into usage_id;
        
      if done then 
        leave read_loop;
      end if;
      
      call sp_tool_charge(usage_id, sysdate(), p_usage_active_time, p_msg);
      
      set p_usage_active_time = 0;
      
    end loop; 
  end main;
    
END //
DELIMITER ;
