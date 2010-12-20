CREATE TABLE "waynodes" (
  "wayid" TEXT NOT NULL,
  "nodeid" TEXT NOT NULL,
   UNIQUE ("wayid","nodeid") ON CONFLICT IGNORE
);
