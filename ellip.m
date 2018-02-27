%Design a 6th-order elliptic bandstop filter with normalized edge frequencies of  and  rad/sample, 5 dB of passband ripple, and 50 dB of stopband attenuation.
%Plot its magnitude and phase responses. Use it to filter random data.
%[b,a] = ellip(3,5,50,[0.2 0.6],'stop');
%freqz(b,a)

rp = 0.3; %passband ripple (dB)
sa = 20; %stopband attentuation (dB)
n = 64; %order of filter
fs = 8000; %sampling frequency
pb1 = 270; %lower frequency of pass band 
pb2 = 450; %higher frequnecy of passband
f = [pb1/(fs/2) pb2/(fs/2)] ; % freq for each transition band
%[b,a] = ellip(n/2,rp,sa,f,'bandpass');
%freqz(b,a);

% Zero-Pole-Gain design 
[z,p,k] = ellip(n/2,rp,sa,f,'bandpass');
sos = zp2sos(z,p,k);
[b,a] = zp2tf(z,p,k);
freqz(sos);

%writing buffer size and coefficient values to a 'fir_coef.txt'
write_ab = fopen('iir_64.txt','wt');
 
%this prints out the length of the buffer required for the filter in C
fprintf(write_ab,'#define N %d\n',length(a));

fprintf(write_ab,'double b[] = {');
%------write b coeffs into array-------------------------
for i = 1: length(b)-1
    fprintf(write_ab,'%e, ',b(i));
end
fprintf(write_ab,'%c}; \n', b(length(b)));

fprintf(write_ab,'double a[] = {');
%------write a coeffs into array-------------------------
for i = 1: length(a)-1
    fprintf(write_ab,'%e, ',a(i));
end
fprintf(write_ab,'%c}; \n', a(length(a)));

fclose('all');


