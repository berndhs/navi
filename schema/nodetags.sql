CREATE TABLE "nodetags" (
  "nodeid" TEXT NOT NULL,
  "key" TEXT NOT NULL ,
  "value" TEXT NOT NULL, 
  UNIQUE ( "nodeid","key") ON CONFLICT REPLACE
);