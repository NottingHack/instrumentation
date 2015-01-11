<?php

include "dbaccess_header.php";

$sql = <<<SQL
  select 
    year(l.access_time) as Year, 
    monthname(l.access_time) as Month, 
    count(*) as "RFID entry",
    (select count(*) from events e1         where year(e1.event_time)=year(l.access_time) and month(e1.event_time)=month(l.access_time) and e1.event_type = "DOORBELL" and (e1.event_value="INNER" or e1.event_value = "")) as "Inner doorbell rang",
    (select count(*) from events e1         where year(e1.event_time)=year(l.access_time) and month(e1.event_time)=month(l.access_time) and e1.event_type = "DOORBELL" and e1.event_value="OUTER") as "Outer doorbell rang",
    (select count(*) from members m         where year(m.join_date  )=year(l.access_time) and month(m.join_date  )=month(l.access_time)) as "New members"
  from access_log l
  where l.access_result = 20
  group by   
    year(l.access_time), 
    monthname(l.access_time)
  order by  
    year(l.access_time) desc, 
    month(l.access_time) desc;
SQL;

print sql_to_tt_json(db_pdo_link(), $sql);

?>