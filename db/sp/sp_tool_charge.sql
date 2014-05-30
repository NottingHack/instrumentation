drop procedure if exists sp_tool_charge;

DELIMITER //
CREATE PROCEDURE sp_tool_charge
(
   IN  p_usage_id           int,
   IN  p_usage_end          datetime,
   IN  p_usage_active_time  int,
   OUT p_msg                varchar(200)
)
SQL SECURITY DEFINER
BEGIN
  
  declare cnt int;
  declare new_usage_duration int;
  declare acc_usage_duration int;
  declare tool_id int;
  declare tool_pph int;
  declare zero_rate int;
  declare full_rate int;
  declare amount int;
  declare tran_desc varchar(50);
  declare tool_name varchar(20);
  declare tool_status varchar(20);
  declare tran_id int;
  declare usage_start datetime;
  declare usage_status varchar(20);
  declare member_id int;
  declare err int;

  -- If anything goes wrong, rollback & exit
  declare exit handler for SQLWARNING, SQLEXCEPTION
  begin 
    set p_msg = concat(p_msg, 'ROLLBACK');
    set err = 1;
    rollback;
  end;
  
  set p_msg = '';
  set err = 0;

  start transaction;
  
  main: begin    

  -- get member_id & tool details
  select 
    tu.member_id, 
    tu.tool_id, 
    tu.usage_start, 
    tu.usage_status,
    t.tool_pph, 
    t.tool_name,
    t.tool_status
  into 
    member_id, 
    tool_id, 
    usage_start, 
    usage_status,
    tool_pph, 
    tool_name,
    tool_status
  from tl_tool_usages tu
  inner join tl_tools t on tu.tool_id = t.tool_id
  where tu.usage_id = p_usage_id;
  
  if (usage_status != 'IN_PROGRESS') then
    set p_msg = 'Incorrect status: not IN_PROGRESS';
    set err = 1;
    leave main;
  end if;
  
  set new_usage_duration = timestampdiff(SECOND, usage_start, p_usage_end);
  
  -- Update the usage row passed in to be complete
  update tl_tool_usages
  set 
    usage_duration    = new_usage_duration,
    usage_active_time = p_usage_active_time,
    usage_status      = 'COMPLETE'
  where
    tl_tool_usages.usage_id = p_usage_id;   
  
 
  -- If there is no charge for using the tool, do nothing
  if (tool_pph = 0) then
    set p_msg = 'No charge for tool';
  
    -- Mark as charged so if a rate ever is set, it doesn't get charged then
    update tl_tool_usages
    set usage_status = 'CHARGED'
    where tl_tool_usages.usage_id = p_usage_id;
    leave main;
  end if;
    
  -- Sum the amount of time waiting to be charged
  select sum(tu.usage_duration)
  into  acc_usage_duration
  from tl_tool_usages tu
  where tu.member_id = member_id
    and tu.tool_id = tool_id
    and tu.usage_status = 'COMPLETE';
  
  -- calc zero-rate charges (i.e. take into account any unspent pledged time)
  if (acc_usage_duration <= 0) then
    set zero_rate = new_usage_duration; -- tool_usage still has a -ve usage time, so no charges apply yet
  else
    set zero_rate = new_usage_duration - acc_usage_duration;
    
    update tl_tool_usages
    set usage_status = 'CHARGED'
    where tl_tool_usages.member_id = member_id
      and tl_tool_usages.tool_id = tool_id;
  end if;
  
  -- Set full rate time
  set full_rate = new_usage_duration - zero_rate;
  
  -- Log zero rate use
  if (zero_rate !=0) then
    set amount = 0;
    set tran_desc = concat('[', sec_to_time(zero_rate), '] of [', tool_name, '] use @ £', format((0)/100, 2), ' p/h');
  
    call sp_transaction_log(member_id, amount, 'TOOL', 'COMPLETE', tran_desc, null, tran_id, p_msg);
    if (length(p_msg) > 0) then
      set err = 1;
      leave main;
    end if;  
  end if;   
  
  -- log full rate use
  if (full_rate != 0) then
    set amount = -1*(full_rate * (tool_pph/3600));
    set tran_desc = concat('[', sec_to_time(full_rate), '] of [', tool_name, '] use @ £', format((tool_pph)/100, 2), ' p/h');  
  
    call sp_transaction_log(member_id, amount, 'TOOL', 'COMPLETE', tran_desc, null, tran_id, p_msg);
    if (length(p_msg) > 0) then
      set err = 1;
      leave main;
    end if;  
  end if;   

  end main;
  
  if (err != 0) then 
    rollback;
  else
    commit;
  end if;
  
END //
DELIMITER ;
