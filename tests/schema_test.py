"""Exercise only module tables in an isolated, disposable MySQL container."""
from pathlib import Path
import argparse,hashlib,json,secrets,subprocess,time
HERE=Path(__file__).resolve().parent
MODULE=HERE.parent
def run(args,**kw):
 p=subprocess.run(args,capture_output=True,timeout=180,**kw)
 if p.returncode:raise RuntimeError(p.stderr.decode(errors='replace')[-1600:])
 return p.stdout
def main():
 parser=argparse.ArgumentParser();parser.add_argument('--image',required=True,help='Available MySQL Docker image or digest');parser.add_argument('--report',type=Path,default=Path('isolated-sql-report.json'));args=parser.parse_args();m={'databaseImage':args.image}
 schema=(MODULE/'data/sql/db-characters/base/001_well_rested.sql').read_bytes()
 name='well-rested-sql-'+secrets.token_hex(6);report={'status':'starting','image':m['databaseImage'],'schemaSha256':hashlib.sha256(schema).hexdigest(),'checks':[]};created=False
 def sql(s):return run(['docker','exec','-i',name,'sh','-c','MYSQL_PWD="$MYSQL_ROOT_PASSWORD" mysql -uroot --batch --skip-column-names'],input=s.encode()).decode().strip()
 try:
  run(['docker','run','-d','--name',name,'--network','none','--tmpfs','/var/lib/mysql','-e','MYSQL_ROOT_PASSWORD='+secrets.token_hex(24),m['databaseImage']]);created=True
  for _ in range(180):
   try:
    sql('SELECT 1;');break
   except RuntimeError:time.sleep(1)
  else:raise RuntimeError('Isolated database did not become ready')
  sql('CREATE DATABASE well_rested_test;');prefix='USE well_rested_test;\n'
  sql(prefix+schema.decode());sql(prefix+schema.decode())
  assert sql(prefix+'SELECT version FROM mod_well_rested_schema WHERE id=1;')=='1'
  report['checks'].append('Schema applies twice without data loss')
  sql(prefix+'INSERT INTO mod_well_rested_character VALUES (1,7200000,8),(2,3600000,24);')
  sql(prefix+'INSERT INTO mod_well_rested_character (guid,remaining_ms,fraction) VALUES (1,7140000,16) ON DUPLICATE KEY UPDATE remaining_ms=VALUES(remaining_ms),fraction=VALUES(fraction);')
  assert sql(prefix+'SELECT remaining_ms,fraction FROM mod_well_rested_character WHERE guid=1;')=='7140000\t16'
  assert sql(prefix+'SELECT remaining_ms,fraction FROM mod_well_rested_character WHERE guid=2;')=='3600000\t24'
  report['checks'].append('Numeric upsert preserves independent character balances and fractional XP')
  sql(prefix+schema.decode());assert sql(prefix+'SELECT COUNT(*) FROM mod_well_rested_character;')=='2'
  sql(prefix+'START TRANSACTION; UPDATE mod_well_rested_character SET remaining_ms=0 WHERE guid=1; ROLLBACK;')
  assert sql(prefix+'SELECT remaining_ms FROM mod_well_rested_character WHERE guid=1;')=='7140000'
  sql(prefix+'DELETE FROM mod_well_rested_character WHERE guid=1;');assert sql(prefix+'SELECT COUNT(*) FROM mod_well_rested_character;')=='1'
  report['checks'].append('InnoDB rollback and scoped deletion preserve the other character')
  sql(prefix+'DROP TABLE mod_well_rested_character; DROP TABLE mod_well_rested_schema;')
  assert sql(prefix+"SELECT COUNT(*) FROM information_schema.TABLES WHERE TABLE_SCHEMA='well_rested_test';")=='0'
  report['status']='passed'
  print('ISOLATED WELL RESTED SQL PASSED. Apply/reapply, save/load values, transaction rollback and cleanup verified. No live database changes. Gameplay still requires testing.')
 finally:
  if created:run(['docker','rm','-f',name])
  args.report.write_text(json.dumps(report,indent=2)+'\n')
if __name__=='__main__':main()
