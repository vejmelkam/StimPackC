#!/usr/bin/octave -q

a = argv();

if(nargin != 1)
  error('Usage: video_latency.m <event_log>');
end

ev_log = load(a{1});

printf("\t\t\t***** Event log analysis script *****\n");
printf("\n\nAnalyzing event log: %s [%d events found].\n\n", a{1}, size(ev_log,1));

# event log constants
PULSE_ACQUIRED = 10;
PULSE_TIMEOUT = 11;
FRAME_DISPLAYED = 6;
VIDEO_FORCED_STOP = 8;
VIDEO_TIMEOUT = 7;

if(any(ev_log(:,2) == PULSE_ACQUIRED))
  ORIGIN_SIGNAL = PULSE_ACQUIRED;
  orig_sig_name = "pulse_acquired";
else
  # analyze video frames from timeout
  ORIGIN_SIGNAL = PULSE_TIMEOUT;
  orig_sig_name = "pulse_timeout";
end

# compute number of frames shown from each video
printf("Video summary (origin signal is %s):\n", orig_sig_name);
latencies = [];
start = 1;
i = 1;
while(1)

  # find pulse acquisition (or timeout)
  ndx = find(ev_log(start:end, 2) == ORIGIN_SIGNAL, 1, "first");
  if(isempty(ndx))
    break
  end
  ndx = ndx + start - 1;

  # compute latency to frame zero
  assert(ev_log(ndx+1, 2) == FRAME_DISPLAYED);
  lat_i = ev_log(ndx+1,1) - ev_log(ndx,1);
  latencies = [latencies; lat_i];
  
  # find the video end
  vid_end = find((ev_log(ndx:end, 2) == VIDEO_FORCED_STOP)
		 + (ev_log(ndx:end, 2) == VIDEO_TIMEOUT), 1, "first") + ndx - 1;

  assert(ev_log(vid_end-1, 2) == FRAME_DISPLAYED);
  printf("Video %d: %d frames time %g [s] latency %g [ms]\n", i,
	 ev_log(vid_end-1, 3), (ev_log(vid_end-1, 1) - ev_log(ndx, 1)) / 1e6,
	 lat_i / 1000);
  start = vid_end + 1;
  i += 1;
end

printf("\n");

printf("Latency summary: mean = %g min = %g max = %g\n",
       mean(latencies), min(latencies), max(latencies));
