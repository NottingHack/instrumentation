<?php

include "dbaccess_header.php";


$start_date  = new DateTime("2010-01-01"); 
$end_date    = new DateTime();
$date_diff   = $start_date->diff($end_date);
$month_count = $date_diff->m + ($date_diff->y * 12);
$month_count++; // Include the current month in the stats as well


$sql = <<<SQL
select
  year(dt.report_date) as Year,
  monthname(dt.report_date) as Month,
  ifnull(new_mem.c, (select count(*) from members m where m.join_date >= dt.report_date and m.join_date < date_add(dt.report_date, interval 1 month))) as "New members",
  ifnull(ex_mem.c, 0)   as "Ex members",
  ifnull(rfid.c, 0)     as "RFID entry",
  ifnull(in_door.c, 0)  as "Inner doorbell rang",
  ifnull(out_door.c, 0) as "Outer doorbell rang"
from 
(
SQL;
$sql .= "select date_add('" . $start_date->format('Y-m-d') . "', interval @num:=@num+1 month) as report_date,";
$sql .= "  cast(@num as signed) as i ";
$sql .= "  from members, (select @num:=-1) num "; // choice of members table is arbitrary, it just needs to be a table with at least $month_count rows in it
$sql .= "  limit $month_count ";
$sql .= <<<SQL
) as dt
left outer join
( -- get details for rfid entry count
  select
    year(l.access_time) as Year,
    monthname(l.access_time) as Month,
    count(*) as c
  from access_log l
  where l.access_result = 20 -- access granted
  group by year(l.access_time), monthname(l.access_time)
) rfid on rfid.Year = year(dt.report_date) and rfid.Month = monthname(dt.report_date)
left outer join
( -- inner door bell
  select
    year(e.event_time) as Year,
    monthname(e.event_time) as Month,
    count(*) as c
  from events e
  where e.event_type = "DOORBELL"
    and (e.event_value="1" or e.event_value = "")
  group by year(e.event_time), monthname(e.event_time)
) in_door on in_door.Year = year(dt.report_date) and in_door.Month = monthname(dt.report_date)
left outer join
( -- outer door bell
  select
    year(e.event_time) as Year,
    monthname(e.event_time) as Month,
    count(*) as c
  from events e
  where e.event_type = "DOORBELL"
    and (e.event_value="2")
  group by year(e.event_time), monthname(e.event_time)
) out_door on out_door.Year = year(dt.report_date) and out_door.Month = monthname(dt.report_date)
left outer join
( -- new members (post HMS intoduction)
  select
    year(s.timestamp) as Year,
    monthname(s.timestamp) as Month,
    count(*) as c
  from status_updates s
  where s.old_status != 5
    and s.new_status = 5
  group by year(s.timestamp), monthname(s.timestamp)
) new_mem on new_mem.Year = year(dt.report_date) and new_mem.Month = monthname(dt.report_date)
left outer join
( -- ex members
  select
    year(s.timestamp) as Year,
    monthname(s.timestamp) as Month,
    count(*) as c
  from status_updates s
  where s.old_status = 5
    and s.new_status = 6
  group by year(s.timestamp), monthname(s.timestamp)
) ex_mem on ex_mem.Year = year(dt.report_date) and ex_mem.Month = monthname(dt.report_date)
order by dt.i desc;
SQL;

print sql_to_tt_json(db_pdo_link(), $sql);

?>