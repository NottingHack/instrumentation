drop procedure if exists sp_tool_induct;

/* Recorded member with card <card_inductee> as having been inducted on <tool_name> by member with card <card_inductee>.
 * ret = 0 on success, non-zero on failure
 * p_msg = mesage to show on LCD */

DELIMITER //
CREATE PROCEDURE sp_tool_induct
(
   IN  tool_name       varchar( 20),
   IN  card_inductor   varchar( 50),
   IN  card_inductee   varchar( 50),
   OUT ret             int,
   OUT p_msg           varchar(200)
)
SQL SECURITY DEFINER
BEGIN
           -- ret: 0 = success, others=failure
  declare cnt int;
  declare inductor_id int;
  declare inductee_id int;
  declare tool_id int;

  set p_msg = '';
  set ret = -1;
  
  main: begin
   
    -- Check tool name is actaully known
    select count(*)
    into cnt
    from tl_tools t
    where t.tool_name = tool_name;
    
    if (cnt = 0) then
      set p_msg = 'Tool not configured';
    leave main;
    elseif (cnt > 1) then
      set p_msg = 'Tool config error';
      leave main;
    end if;
    
    -- Get tool id 
    select t.tool_id
    into tool_id
    from tl_tools t
    where t.tool_name = tool_name;
    
    -- Check <card_inductor> is actaully listed as being able to give inductions, and get details if so
    select count(*)
    into cnt
    from members m
    inner join rfid_tags r on m.member_id = r.member_id
    inner join tl_members_tools mt on mt.member_id = m.member_id
    inner join tl_tools tl on tl.tool_id = mt.tool_id
    where r.rfid_serial = card_inductor
      and tl.tool_name = tool_name
      and mt.mt_access_level in ('INDUCTOR','MAINTAINER');
      
    if (cnt <= 0) then
      set p_msg = 'Access denied    (NI)';
      leave main;
    end if;
    
    -- Get member id of inductor
    select m.member_id
    into inductor_id
    from members m
    inner join rfid_tags r on r.member_id = m.member_id
    where r.rfid_serial = card_inductor;


    -- check <card_inductee> relates to a current member
    select count(*)
    into cnt
    from members m
    inner join rfid_tags r on m.member_id = r.member_id
    where r.rfid_serial = card_inductee
      and r.state = 10
      and m.member_status = 5; -- CURRENT MEMBER
    
    if (cnt <= 0) then
      set p_msg = 'Failed: bad card';
      leave main;
    end if;

    
    -- Check if inductee has already been inducted.. and just return success if so
    select count(*)
    into cnt
    from members m
    inner join rfid_tags r on m.member_id = r.member_id
    inner join tl_members_tools mt on mt.member_id = m.member_id
    inner join tl_tools tl on tl.tool_id = mt.tool_id
    where r.rfid_serial = card_inductee
      and tl.tool_name = tool_name;
      
    if (cnt > 0) then
      set p_msg = 'Already inducted';
      set ret = 0;
      leave main;
    end if;
    
    -- Get member id of inductee
    select m.member_id
    into inductee_id
    from members m
    inner join rfid_tags r on r.member_id = m.member_id
    where r.rfid_serial = card_inductee;

    -- Add induction record
    insert into tl_members_tools (member_id  , tool_id, member_id_induct, mt_date_inducted, mt_access_level)
                          values (inductee_id, tool_id, inductor_id     , sysdate()       , 'USER'); 
    set ret = 0;


  end main;
    
END //
DELIMITER ;
