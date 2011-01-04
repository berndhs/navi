CREATE TABLE "waynodes" (
  "wayid" INTEGER PRIMARY KEY NOT NULL,
  "nodeid" TEXT NOT NULL,
   UNIQUE ("wayid","nodeid") ON CONFLICT IGNORE
);
